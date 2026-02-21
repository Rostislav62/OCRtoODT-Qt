// ============================================================
//  OCRtoODT — Input Processor
//  File: src/core/processors/InputProcessor.cpp
// ============================================================

#include "core/processors/InputProcessor.h"

// ------------------------------------------------------------
// STEP 0
// ------------------------------------------------------------
#include "0_input/InputController.h"
#include "0_input/PreviewController.h"

// ------------------------------------------------------------
// STEP 1
// ------------------------------------------------------------
#include "1_preprocess/Preprocess_Pipeline.h"

// ------------------------------------------------------------
// Project
// ------------------------------------------------------------
#include "core/ConfigManager.h"
#include "core/LogRouter.h"
#include "core/ThemeManager.h"

// ------------------------------------------------------------
// Qt
// ------------------------------------------------------------
#include <QListView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QTimer>
#include <QDir>
#include <QImage>
#include <QPixmap>
#include <QIcon>

// ------------------------------------------------------------
// OpenCV (USED ONLY IN .cpp)
// ------------------------------------------------------------
#include <opencv2/core.hpp>

// ============================================================
// Helper: cv::Mat (Gray8) → QImage
// ============================================================
static QImage grayMatToQImage(const cv::Mat &mat)
{
    if (mat.empty() || mat.type() != CV_8UC1)
        return QImage();

    QImage img(mat.cols, mat.rows, QImage::Format_Grayscale8);

    for (int y = 0; y < mat.rows; ++y)
        memcpy(img.scanLine(y), mat.ptr(y), static_cast<size_t>(mat.cols));

    return img;
}

// ============================================================
// Constructor
// ============================================================
InputProcessor::InputProcessor(QObject *parent)
    : QObject(parent)
{
    m_inputController    = new Input::InputController(this);
    m_preprocessPipeline = new Ocr::Preprocess::PreprocessPipeline(this);

    m_step1PollTimer = new QTimer(this);
    m_step1PollTimer->setInterval(150);
}

// ============================================================
// Access prepared input pages (STEP 0 output)
// ============================================================
const QVector<Core::VirtualPage>& InputProcessor::pages() const
{
    return m_pages;
}

// ============================================================
// Attach UI
// ============================================================
void InputProcessor::attachUi(QListView *listFiles,
                              Input::PreviewController *previewController)
{
    m_listFiles         = listFiles;
    m_previewController = previewController;

    // Apply thumbnail size immediately from config (clamped 100..200)
    applyThumbnailSizeFromConfig();

    // Re-apply whenever ThemeManager reloads settings (Settings -> OK)
    connect(ThemeManager::instance(),
            &ThemeManager::thumbnailSizeChanged,
            this,
            [this](int)
            {
                applyThumbnailSizeFromConfig();
            });

    wire();
}


// ============================================================
// Wiring
// ============================================================
void InputProcessor::wire()
{
    if (!m_listFiles || !m_previewController)
        return;

    // STEP 0 → model
    connect(m_inputController, &Input::InputController::filesLoaded,
            this,
            [this](QStandardItemModel *model)
            {
                m_model = model;
                m_expectedPages = model ? model->rowCount() : 0;

                // STEP 0 output snapshot invalid until we rebuild it
                m_pages.clear();

                m_listFiles->setModel(model);

                if (model && model->rowCount() > 0)
                {
                    QModelIndex first = model->index(0, 0);
                    m_listFiles->setCurrentIndex(first);
                    m_inputController->handleItemActivated(first);
                }

                startStep1Polling();
                emit inputStateChanged();
            });

    // STEP 0 → preview
    connect(m_inputController, &Input::InputController::previewReady,
            this,
            [this](const Core::VirtualPage &vp, const QImage &img)
            {
                showPreviewAccordingToConfig(vp, img);
            });

    // list click → STEP 0 preview
    connect(m_listFiles, &QListView::clicked,
            m_inputController, &Input::InputController::handleItemActivated);

    // STEP 1 polling
    connect(m_step1PollTimer, &QTimer::timeout,
            this,
            [this]()
            {
                if (m_step1Running)
                    return;

                if (!isStep0Complete())
                    return;

                m_step1PollTimer->stop();

                // Build stable STEP 0 output snapshot
                const QVector<Core::VirtualPage> &pages = rebuildPagesFromCacheAndModel();
                runStep1Preprocess(pages);
            });

    connect(m_inputController,
            &Input::InputController::pageActivated,
            this,
            &InputProcessor::pageActivated);


}

// ============================================================
// Scenario entry
// ============================================================
void InputProcessor::run(QWidget *parentWidget)
{
    if (m_step1Running)
    {
        LogRouter::instance().warning("[InputProcessor] run() ignored: STEP1 running");
        return;
    }

    if (m_step1PollTimer && m_step1PollTimer->isActive())
    {
        LogRouter::instance().warning("[InputProcessor] run() ignored: STEP0 in progress");
        return;
    }

    ConfigManager &cfg = ConfigManager::instance();
    m_showFinalPreview =
        cfg.get("preprocess.show_final_preview", true).toBool();

    m_jobsByIndex.clear();
    m_pages.clear();

    m_step1Running = false;

    LogRouter::instance().info("[InputProcessor] STEP 0_input");
    m_inputController->openFiles(parentWidget);
}

// ============================================================
// STEP 1 helpers
// ============================================================
void InputProcessor::startStep1Polling()
{
    if (m_expectedPages > 0)
        m_step1PollTimer->start();
}

bool InputProcessor::isStep0Complete() const
{
    if (!m_model || m_expectedPages <= 0)
        return false;

    for (int i = 0; i < m_expectedPages; ++i)
    {
        QStandardItem *it = m_model->item(i);
        if (!it || it->text().startsWith("Page"))
            return false;
    }

    return true;
}

// ------------------------------------------------------------
// Build STEP 0 output pages snapshot into m_pages
// ------------------------------------------------------------
const QVector<Core::VirtualPage>& InputProcessor::rebuildPagesFromCacheAndModel()
{
    m_pages.clear();

    if (!m_model || m_expectedPages <= 0)
        return m_pages;

    m_pages.resize(m_expectedPages);

    ConfigManager &cfg = ConfigManager::instance();
    const QString dirName = cfg.get("general.input_dir", "input").toString();
    const QString base = QDir::currentPath() + "/cache/" + dirName;

    for (int i = 0; i < m_expectedPages; ++i)
    {
        const QStandardItem *it = m_model->item(i);
        const QString name = it ? it->text() : QString();

        Core::VirtualPage vp;
        vp.sourcePath   = QDir(base).filePath(name);
        vp.displayName  = name;
        vp.setGlobalIndex(i);

        m_pages[i] = vp;
    }

    LogRouter::instance().info(
        QString("[InputProcessor] STEP 0 pages snapshot built: %1").arg(m_pages.size()));

    return m_pages;
}

void InputProcessor::runStep1Preprocess(const QVector<Core::VirtualPage> &pages)
{
    if (pages.isEmpty())
        return;

    m_step1Running = true;

    LogRouter::instance().info(
        QString("[InputProcessor] STEP 1_preprocess (pages=%1)").arg(pages.size()));

    QVector<Ocr::Preprocess::PageJob> jobs =
        m_preprocessPipeline->run(pages);

    m_jobsByIndex.clear();
    for (const auto &job : jobs)
        m_jobsByIndex.insert(job.globalIndex, job);

    if (m_showFinalPreview)
        applyEnhancedThumbnails();

    if (m_showFinalPreview && m_listFiles->currentIndex().isValid())
        m_inputController->handleItemActivated(m_listFiles->currentIndex());

    m_step1Running = false;

    LogRouter::instance().info(
        QString("[InputProcessor] STEP 1 finished (jobs=%1)").arg(jobs.size()));
}

void InputProcessor::applyEnhancedThumbnails()
{
    if (!m_model || !m_listFiles)
        return;

    QSize iconSize = m_listFiles->iconSize();

    for (int i = 0; i < m_model->rowCount(); ++i)
    {
        if (!m_jobsByIndex.contains(i))
            continue;

        const auto &job = m_jobsByIndex[i];

        QImage img = (!job.enhancedMat.empty())
                         ? grayMatToQImage(job.enhancedMat)
                         : QImage(job.enhancedPath);

        if (img.isNull())
            continue;

        QPixmap pix = QPixmap::fromImage(img)
                          .scaled(iconSize, Qt::KeepAspectRatio,
                                  Qt::SmoothTransformation);
        m_model->item(i)->setIcon(QIcon(pix));
    }
}

void InputProcessor::showPreviewAccordingToConfig(
    const Core::VirtualPage &vp,
    const QImage &originalImg)
{
    if (!m_showFinalPreview ||
        !m_jobsByIndex.contains(vp.getGlobalIndex()))
    {
        m_previewController->setPreviewImage(vp, originalImg);
        return;
    }

    const auto &job = m_jobsByIndex[vp.getGlobalIndex()];

    QImage img = (!job.enhancedMat.empty())
                     ? grayMatToQImage(job.enhancedMat)
                     : QImage(job.enhancedPath);

    if (img.isNull())
        img = originalImg;

    m_previewController->setPreviewImage(vp, img);
}

// ============================================================
// Full session reset
// ============================================================
void InputProcessor::clearSession()
{
    LogRouter::instance().info("[InputProcessor] Clearing session");

    // SAFETY: do not allow clear while STEP 1 is running
    if (m_step1Running)
    {
        LogRouter::instance().warning("[InputProcessor] Clear ignored: STEP 1 is running");
        return;
    }

    // Stop STEP 1 polling timer
    if (m_step1PollTimer && m_step1PollTimer->isActive())
        m_step1PollTimer->stop();

    m_step1Running = false;

    // Clear STEP 1 RAM data
    m_jobsByIndex.clear();

    // Clear STEP 0 output snapshot
    m_pages.clear();

    // Detach UI from model FIRST
    if (m_listFiles)
        m_listFiles->setModel(nullptr);

    if (m_previewController)
        m_previewController->setPreviewImage(Core::VirtualPage{}, QImage());

    // Clear STEP 0 state
    m_expectedPages = 0;
    m_model = nullptr;

    // Reset InputController
    if (m_inputController)
        m_inputController->reset();

    // Remove cache/ directory completely
    const QString cachePath = QDir::currentPath() + "/cache";
    QDir cacheDir(cachePath);

    if (cacheDir.exists())
    {
        cacheDir.removeRecursively();
        LogRouter::instance().info("[InputProcessor] cache/ removed");
    }

    emit inputStateChanged();
}


QVector<Ocr::Preprocess::PageJob> InputProcessor::preprocessJobs() const
{
    QVector<Ocr::Preprocess::PageJob> jobs;
    jobs.reserve(m_jobsByIndex.size());

    for (auto it = m_jobsByIndex.begin(); it != m_jobsByIndex.end(); ++it)
        jobs.push_back(it.value());

    return jobs;
}

void InputProcessor::applyThumbnailSizeFromConfig()
{
    if (!m_listFiles)
        return;

    ConfigManager &cfg = ConfigManager::instance();

    int s = cfg.get("ui.thumbnail_size", 160).toInt();
    if (s < 100) s = 100;
    if (s > 200) s = 200;

    m_listFiles->setIconSize(QSize(s, s));

    // If STEP1 final preview thumbnails are enabled and already computed,
    // rebuild icons to match new iconSize (crisp instead of view-scaling).
    if (m_showFinalPreview && !m_jobsByIndex.isEmpty())
        applyEnhancedThumbnails();
}
