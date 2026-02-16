// ============================================================
//  OCRtoODT — Input Controller
//  File: src/0_input/InputController.cpp
// ============================================================

#include "0_input/InputController.h"
#include "0_input/ImageThumbnailProvider.h"

#include "core/VirtualPage.h"
#include "core/ConfigManager.h"
#include "core/LogRouter.h"   // ✅ centralized logging

#include <QFileDialog>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QImageReader>
#include <QtConcurrent>
#include <QMetaType>

// PDF
#include <poppler/qt6/poppler-qt6.h>
#include <memory>

using namespace Input;

// ============================================================
// Constructor
// ============================================================
InputController::InputController(QObject *parent)
    : QObject(parent)
{
    // REQUIRED for queued previewReady()
    qRegisterMetaType<Core::VirtualPage>("Core::VirtualPage");

    m_thumbProvider = new ImageThumbnailProvider(this);

    connect(m_thumbProvider, &ImageThumbnailProvider::thumbnailReady,
            this, &InputController::onThumbnailReady);

    connect(&m_watcher, &QFutureWatcher<PageResult>::resultReadyAt,
            this, &InputController::onJobResultReady);

    connect(&m_watcher, &QFutureWatcher<PageResult>::finished,
            this, &InputController::onJobsFinished);
}

InputController::~InputController()
{
    reset();
}

// ============================================================
// STEP 0_input entry
// ============================================================
void InputController::openFiles(QWidget *parentWidget)
{
    const QStringList paths =
        QFileDialog::getOpenFileNames(
            parentWidget,
            tr("Open images or PDF"),
            QString(),
            tr("Images/PDF (*.png *.jpg *.jpeg *.bmp *.tif *.tiff *.pdf)")
            );

    if (paths.isEmpty())
        return;

    reset();

    QVector<PageWorkItem> items;
    int seq = 0;

    for (const QString &p : paths)
    {
        if (p.endsWith(".pdf", Qt::CaseInsensitive))
        {
            std::unique_ptr<Poppler::Document> doc(
                Poppler::Document::load(p));

            if (!doc)
                continue;

            for (int i = 0; i < doc->numPages(); ++i)
                items.push_back({ p, true, i, seq++ });
        }
        else
        {
            items.push_back({ p, false, -1, seq++ });
        }
    }

    if (items.isEmpty())
        return;

    // STEP 1: create empty UI list immediately
    initializePlaceholders(items.size());

    // STEP 2: process first page synchronously (instant preview)
    PageResult first = processSingleItem(items.first());
    if (first.ok)
    {
        finalizePage(first);

        QModelIndex idx = m_model->index(0, 0);
        handleItemActivated(idx);
    }

    // STEP 3: process remaining pages asynchronously
    if (items.size() > 1)
    {
        QVector<PageWorkItem> rest = items.mid(1);

        auto fn = [this](const PageWorkItem &it)
        {
            return processSingleItem(it);
        };

        m_watcher.setFuture(QtConcurrent::mapped(rest, fn));
    }
}

// ============================================================
// Create placeholder list
// ============================================================
void InputController::initializePlaceholders(int totalPages)
{
    m_pages.clear();
    m_pages.resize(totalPages);

    delete m_model;
    m_model = new QStandardItemModel(this);

    for (int i = 0; i < totalPages; ++i)
    {
        auto *item = new QStandardItem(tr("Page %1").arg(i + 1));
        m_model->appendRow(item);
    }

    emit filesLoaded(m_model);
}

// ============================================================
// Process single page
// ============================================================
InputController::PageResult
InputController::processSingleItem(const PageWorkItem &it)
{
    PageResult out;
    out.sequenceIndex = it.sequenceIndex;

    ConfigManager &cfg = ConfigManager::instance();
    const QString inputDirName =
        cfg.get("general.input_dir", "input").toString();

    const QString baseDir = QDir::currentPath();
    const QString inputDir = baseDir + "/cache/" + inputDirName;

    QDir().mkpath(inputDir);

    const QString fileName =
        QString("%1.png").arg(it.sequenceIndex + 1, 4, 10, QLatin1Char('0'));

    const QString dstPath = inputDir + "/" + fileName;

    if (!it.isPdf)
    {
        QFileInfo fi(it.path);
        if (!fi.exists() || !fi.isFile())
            return out;

        const QString ext = fi.suffix();
        const QString realName =
            QString("%1.%2")
                .arg(it.sequenceIndex + 1, 4, 10, QLatin1Char('0'))
                .arg(ext);

        const QString realDst = inputDir + "/" + realName;

        if (QFile::exists(realDst))
            QFile::remove(realDst);

        if (!QFile::copy(it.path, realDst))
            return out;

        QImageReader reader(realDst);

        out.finalPath   = realDst;
        out.displayName = realName;
        out.imgWidth    = reader.size().width();
        out.imgHeight   = reader.size().height();
        out.imgFormat   = ext.toLower();
        out.ok          = true;

        return out;
    }

    std::unique_ptr<Poppler::Document> doc(
        Poppler::Document::load(it.path));
    if (!doc)
        return out;

    std::unique_ptr<Poppler::Page> page(doc->page(it.pdfPageIndex));
    if (!page)
        return out;

    QImage img = page->renderToImage(300, 300);
    if (img.isNull())
        return out;

    if (!img.save(dstPath))
        return out;

    out.finalPath   = dstPath;
    out.displayName = fileName;
    out.imgWidth    = img.width();
    out.imgHeight   = img.height();
    out.imgFormat   = "png";
    out.ok          = true;

    return out;
}

// ============================================================
// Apply PageResult to model + thumbnails
// ============================================================
void InputController::finalizePage(const PageResult &res)
{
    if (!res.ok || res.sequenceIndex < 0 || res.sequenceIndex >= m_pages.size())
        return;

    Core::VirtualPage vp;
    vp.sourcePath  = res.finalPath;
    vp.displayName = res.displayName;
    vp.setGlobalIndex(res.sequenceIndex);

    m_pages[res.sequenceIndex] = vp;

    QStandardItem *item = m_model->item(res.sequenceIndex);
    if (!item)
        return;

    item->setText(vp.displayName);

    ConfigManager &cfg = ConfigManager::instance();

    int s = cfg.get("ui.thumbnail_size", 160).toInt();
    if (s < 100) s = 100;
    if (s > 200) s = 200;

    m_thumbProvider->requestThumbnail(vp.sourcePath, QSize(s, s));

}

// ============================================================
// Thumbnail ready → set icon
// ============================================================
void InputController::onThumbnailReady(const QString &key,
                                       const QPixmap &pix)
{
    for (int i = 0; i < m_model->rowCount(); ++i)
    {
        const Core::VirtualPage &vp = m_pages[i];
        if (vp.sourcePath == key)
        {
            QStandardItem *item = m_model->item(i);
            if (item)
                item->setIcon(QIcon(pix));
            return;
        }
    }
}

// ============================================================
// Page activation handler (🔑 sync point)
// ============================================================
void InputController::handleItemActivated(const QModelIndex &index)
{
    if (!index.isValid() || index.row() >= m_pages.size())
        return;

    const Core::VirtualPage &vp = m_pages[index.row()];
    if (vp.sourcePath.isEmpty())
        return;

    const int gi = vp.globalIndex;

    // Emit page activation FIRST (Text Tab sync)
    emit pageActivated(gi);

    // Emit preview asynchronously
    QtConcurrent::run([this, vp]()
                      {
                          QImage img(vp.sourcePath);
                          if (!img.isNull())
                              emit previewReady(vp, img);
                      });
}

// ============================================================
// Async results
// ============================================================
void InputController::onJobResultReady(int index)
{
    PageResult res = m_watcher.future().resultAt(index);
    if (res.ok)
        finalizePage(res);
}

void InputController::onJobsFinished()
{
    // Unified logging instead of qDebug
    LogRouter::instance().info(
        "[InputController] All pages processed");
}

// ============================================================
// Reset
// ============================================================
void InputController::reset()
{
    if (m_watcher.isRunning())
    {
        m_watcher.cancel();
        m_watcher.waitForFinished();
    }

    m_pages.clear();

    delete m_model;
    m_model = nullptr;
}
