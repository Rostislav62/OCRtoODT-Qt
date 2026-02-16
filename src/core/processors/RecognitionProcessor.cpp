// ============================================================
//  OCRtoODT — Recognition Processor
//  File: src/core/processors/RecognitionProcessor.cpp
// ============================================================

#include "core/processors/RecognitionProcessor.h"

#include <QFile>
#include <QDir>

#include "2_ocr/OcrPipeLineController.h"

#include "3_LineTextBuilder/LineTextBuilder.h"
#include "3_LineTextBuilder/LineTableSerializer.h"

#include "core/ConfigManager.h"
#include "core/LogRouter.h"
#include "core/VirtualPage.h"
#include "core/ProgressManager.h"


// ============================================================
// Constructor / Destructor
// ============================================================

RecognitionProcessor::RecognitionProcessor(QObject *parent)
    : QObject(parent)
{

    ensureControllers();
}

RecognitionProcessor::~RecognitionProcessor()
{
    clearOldLineTables();
}

// ============================================================
// Controller setup
// ============================================================

void RecognitionProcessor::ensureControllers()
{
    if (m_ocrController)
        return;

    m_ocrController = new Ocr::OcrPipelineController(this);

    connect(m_ocrController, &Ocr::OcrPipelineController::ocrMessage,
            this, &RecognitionProcessor::ocrMessage);

    connect(m_ocrController, &Ocr::OcrPipelineController::ocrFinished,
            this, &RecognitionProcessor::ocrFinished);

    // OCR finished → receive VirtualPage[]
    connect(m_ocrController, &Ocr::OcrPipelineController::ocrCompleted,
            this, &RecognitionProcessor::onOcrCompletedFromOcr,
            Qt::QueuedConnection);

    connect(m_ocrController,
            &Ocr::OcrPipelineController::ocrCompleted,
            this,
            [this]()
            {
                if (!m_progressManager)
                    return;

                m_progressManager->advance(m_jobs.size());
            });

    connect(m_ocrController,
            &Ocr::OcrPipelineController::ocrProgress,
            this,
            [this](int done, int total)
            {
                if (!m_progressManager)
                    return;

                m_progressManager->advance(done - m_lastOcrDone);
                m_lastOcrDone = done;

            });

}


void RecognitionProcessor::setProgressManager(Core::ProgressManager *pm)
{
    m_progressManager = pm;
}


// ============================================================
// Cleanup helper
// ============================================================

void RecognitionProcessor::clearOldLineTables()
{

    for (Core::VirtualPage &vp : m_pages)
    {

        if (vp.lineTable)
        {
            delete vp.lineTable;
            vp.lineTable = nullptr;
        }
    }
}

// ============================================================
// Input
// ============================================================

void RecognitionProcessor::setJobs(const QVector<Ocr::Preprocess::PageJob> &jobs)
{
    m_jobs = jobs;
}

// ============================================================
// Run STEP 2
// ============================================================

void RecognitionProcessor::run()
{
    emit processingStarted();
    if (m_progressManager)
    {
        m_lastOcrDone = 0;
        m_progressManager->startPipeline(2);
        m_progressManager->startStage(tr("OCR"), 0, 2, m_jobs.size());
    }

    if (m_jobs.isEmpty())
    {
        LogRouter::instance().warning(
            "[RecognitionProcessor] STEP 2 aborted: no jobs");
        emit ocrFinished();
        return;
    }

    LogRouter::instance().info(
        QString("[RecognitionProcessor] STEP 2 start (jobs=%1)")
            .arg(m_jobs.size()));

    m_ocrController->start(m_jobs);
}

// ============================================================
// STEP 2 completed → orchestrate STEP 3
// ============================================================

void RecognitionProcessor::onOcrCompletedFromOcr(
    const QVector<Core::VirtualPage> &pages)
{
    LogRouter::instance().info(
        QString("[RecognitionProcessor] OCR completed (pages=%1)")
            .arg(pages.size()));

    // --------------------------------------------------------
    // Replace snapshot (and cleanup previous)
    // --------------------------------------------------------
    clearOldLineTables();
    m_pages = pages;

    // --------------------------------------------------------
    // Global execution mode
    // --------------------------------------------------------
    ConfigManager &cfg = ConfigManager::instance();

    const QString mode =
        cfg.get("general.mode", "ram_only").toString();

    const bool debugMode =
        cfg.get("general.debug_mode", false).toBool();

    LogRouter::instance().info(
        QString("[RecognitionProcessor] STEP 3 config: mode=%1 debug=%2")
            .arg(mode)
            .arg(debugMode));

    // --------------------------------------------------------
    // STEP 3: TSV → LineTable (RAM), optional disk I/O
    // --------------------------------------------------------
    int built  = 0;
    int loaded = 0;
    int saved  = 0;

    const QString baseDir = "cache/line_text";
    QDir().mkpath(baseDir);

    m_lastOcrDone = 0;
    if (m_progressManager)
        m_progressManager->startStage(tr("Line building"), 1, 2, m_pages.size());

    for (Core::VirtualPage &vp : m_pages)
    {
        if (m_progressManager)
            m_progressManager->advance(1);

        if (!vp.ocrSuccess)
        {
            LogRouter::instance().warning(
                QString("[STEP 3] Skip page=%1 (ocrSuccess=false)")
                    .arg(vp.globalIndex));
            continue;
        }

        if (vp.ocrTsvText.isEmpty())
        {
            LogRouter::instance().warning(
                QString("[STEP 3] Skip page=%1 (empty TSV)")
                    .arg(vp.globalIndex));
            continue;
        }

        const QString filePath =
            QString("%1/page_%2.line_table.tsv")
                .arg(baseDir)
                .arg(vp.globalIndex, 4, 10, QLatin1Char('0'));

        // Defensive cleanup
        if (vp.lineTable)
        {
            delete vp.lineTable;
            vp.lineTable = nullptr;
        }

        // DISK_ONLY: try load first
        if (mode == "disk_only" && QFile::exists(filePath))
        {
            vp.lineTable = Tsv::LineTableSerializer::loadFromTsv(filePath);

            if (vp.lineTable)
            {
                ++loaded;
                LogRouter::instance().info(
                    QString("[STEP 3] Loaded LineTable from disk page=%1")
                        .arg(vp.globalIndex));

                if (m_progressManager)
                    m_progressManager->advance(1);
                continue;
            }

            LogRouter::instance().warning(
                QString("[STEP 3] Failed to load LineTable, rebuilding page=%1")
                    .arg(vp.globalIndex));
        }

        // Build in RAM
        vp.lineTable = Tsv::LineTextBuilder::build(vp, vp.ocrTsvText);
        ++built;

        if (m_progressManager)
            m_progressManager->advance(1);

        LogRouter::instance().info(
            QString("[STEP 3] Built LineTable in RAM page=%1 rows=%2")
                .arg(vp.globalIndex)
                .arg(vp.lineTable ? vp.lineTable->rows.size() : 0));

        // Save to disk (debug or disk_only)
        if (debugMode || mode == "disk_only")
        {
            if (vp.lineTable &&
                Tsv::LineTableSerializer::saveToTsv(*vp.lineTable, filePath))
            {
                ++saved;
                LogRouter::instance().info(
                    QString("[STEP 3] LineTable written to disk page=%1")
                        .arg(vp.globalIndex));
            }
            else
            {
                LogRouter::instance().warning(
                    QString("[STEP 3] Failed to write LineTable page=%1")
                        .arg(vp.globalIndex));
            }
        }
    }

    LogRouter::instance().info(
        QString("[RecognitionProcessor] STEP 3 summary: built=%1 loaded=%2 saved=%3")
            .arg(built)
            .arg(loaded)
            .arg(saved));

    if (m_progressManager)
        m_progressManager->finishPipeline(true, tr("OCR completed"));

    emit processingFinished();

    // IMPORTANT: emit pages owned by RecognitionProcessor
    emit ocrCompleted(m_pages);

}

void RecognitionProcessor::cancel()
{
    if (m_ocrController)
        m_ocrController->cancel();

    if (m_progressManager)
    {
        m_progressManager->finishPipeline(false, tr("Cancelled"));
        m_progressManager->reset();   // 🔥 ДОБАВИТЬ
    }

    emit processingFinished();
}



