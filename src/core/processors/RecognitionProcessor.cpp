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

    m_watchdogTimer = new QTimer(this);
    m_watchdogTimer->setSingleShot(true);

    connect(m_watchdogTimer,
            &QTimer::timeout,
            this,
            [this]()
            {
                LogRouter::instance().error(
                    "[RecognitionProcessor] WATCHDOG TIMEOUT — forcing abort.");

                // force-stop worker thread
                if (m_ocrController)
                    m_ocrController->shutdownAndWait();

                finalizeOnce(FinalStatus::Timeout, tr("OCR timeout"));
            });

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

    // OCR finished → receive VirtualPage[]
    connect(m_ocrController, &Ocr::OcrPipelineController::ocrCompleted,
            this, &RecognitionProcessor::onOcrCompletedFromOcr,
            Qt::QueuedConnection);


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

void RecognitionProcessor::resetFinalizationState()
{
    m_finalized = false;

    // Stage 5 hardening:
    // Reset run-phase markers for the new run.
    m_phase = RunPhase::Idle;
    m_seenOcrCompleted = false;

    // Reset progress delta tracking (defensive; Step2 will set it anyway).
    m_lastOcrDone = 0;
}


void RecognitionProcessor::finalizeOnce(FinalStatus status, const QString &reason)
{
    // EXACTLY-ONCE barrier
    // Prevent double-finalization from racing signals.
    if (m_finalized)
        return;

    // If finalize is called without active processing,
    // it means logic error or shutdown edge case.
    if (!m_isProcessing && status != FinalStatus::Shutdown)
    {
        LogRouter::instance().warning(
            "[RecognitionProcessor] finalizeOnce() called while not processing.");
    }

    m_finalized = true;

    // stop watchdog
    if (m_watchdogTimer && m_watchdogTimer->isActive())
        m_watchdogTimer->stop();

    // normalize processing flag
    m_isProcessing = false;

    // finalize progress
    if (m_progressManager)
    {
        const bool ok = (status == FinalStatus::Success);

        QString msg = reason;
        if (msg.isEmpty())
        {
            switch (status)
            {
            case FinalStatus::Success:   msg = tr("OCR completed"); break;
            case FinalStatus::Cancelled: msg = tr("Cancelled");     break;
            case FinalStatus::Timeout:   msg = tr("OCR timeout");   break;
            case FinalStatus::NoJobs:    msg = tr("No jobs");       break;
            case FinalStatus::Shutdown:  msg = tr("Shutdown");      break;
            }
        }

        m_progressManager->finishPipeline(ok, msg);
    }

    emit processingFinished();
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
    // Defensive guard:
    // Jobs cannot be replaced while processing is active.
    // This prevents state corruption and undefined pipeline behaviour.
    if (m_isProcessing)
    {
        LogRouter::instance().warning(
            "[RecognitionProcessor] setJobs() ignored: processing active.");
        return;
    }

    m_jobs = jobs;
}

// ============================================================
// Run STEP 2
// CONTRACT:
// - setJobs() must be called before run()
// - run() may be called only when isProcessing() == false
// - After clearSession(), jobs must be set again
// ============================================================

void RecognitionProcessor::run()
{
    if (m_isProcessing)
    {
        LogRouter::instance().warning(
            "[RecognitionProcessor] run() ignored: already processing");
        return;
    }

    // Defensive guard:
    // Running without jobs is invalid state unless explicitly reconfigured.
    // Prevent accidental re-run with stale state.
    if (m_jobs.isEmpty())
    {
        LogRouter::instance().warning(
            "[RecognitionProcessor] run() ignored: no jobs configured.");
        return;
    }

    resetFinalizationState();

    // Stage 5 hardening: entering STEP2 (OCR running)
    m_phase = RunPhase::Step2_OcrRunning;

    m_isProcessing = true;
    emit processingStarted();

    if (m_progressManager)
    {
        m_lastOcrDone = 0;
        m_progressManager->startPipeline(2);
        m_progressManager->startStage(tr("OCR"), 0, 2, m_jobs.size());
    }


    LogRouter::instance().info(
        QString("[RecognitionProcessor] STEP 2 start (jobs=%1)")
            .arg(m_jobs.size()));

    // Start watchdog (timeout per batch)
    const int timeoutSec =
        ConfigManager::instance()
            .get("general.ocr_timeout_sec", 600)
            .toInt();

    m_watchdogTimer->start(timeoutSec * 1000);

    m_ocrController->start(m_jobs);
}

// ============================================================
// STEP 2 completed → orchestrate STEP 3
// ============================================================

void RecognitionProcessor::onOcrCompletedFromOcr(
    const QVector<Core::VirtualPage> &pages)
{
    LogRouter::instance().info(
        QString("[RecognitionProcessor] onOcrCompletedFromOcr: pages=%1").arg(pages.size()));
    if (!pages.isEmpty())
    {
        LogRouter::instance().info(
            QString("[RecognitionProcessor] sample page0: idx=%1 success=%2 tsv=%3")
                .arg(pages[0].globalIndex)
                .arg(pages[0].ocrSuccess)
                .arg(pages[0].ocrTsvText.size()));
    }

    // Defensive guard:
    // Ignore late completion if already finalized (race safety).
    if (m_finalized)
        return;

    // Stage 5 hardening:
    // We have entered STEP3. From this point ocrFinished must NOT cancel the run.
    m_phase = RunPhase::Step3_LineBuilding;
    m_seenOcrCompleted = true;

    if (m_watchdogTimer->isActive())
        m_watchdogTimer->stop();

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
        // Stage 5 hardening:
        // Each page consumes exactly 1 progress unit in STEP3,
        // even if skipped. This guarantees deterministic completion.
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
                continue;
            }

            LogRouter::instance().warning(
                QString("[STEP 3] Failed to load LineTable, rebuilding page=%1")
                    .arg(vp.globalIndex));
        }

        // Build in RAM
        vp.lineTable = Tsv::LineTextBuilder::build(vp, vp.ocrTsvText);
        ++built;

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

    int withTable = 0;
    for (const auto &vp : m_pages)
        if (vp.lineTable) ++withTable;

    LogRouter::instance().info(
        QString("[RecognitionProcessor] STEP3 done: pages=%1 withLineTable=%2")
            .arg(m_pages.size())
            .arg(withTable));

    // Stage 5 hardening:
    // Finalize FIRST (flip flags, finish progress), then publish results.
    finalizeOnce(FinalStatus::Success, tr("OCR completed"));

    emit ocrCompleted(m_pages);
}

void RecognitionProcessor::cancel()
{
    if (!m_isProcessing)
        return;

    LogRouter::instance().info(
        "[RecognitionProcessor] Cancel requested.");

    // Stage 5 hardening:
    // mark that we are no longer expecting success path from STEP2.
    // (Finalization will happen via ocrFinished fallback if STEP2 is active.)

    if (m_ocrController)
        m_ocrController->cancel();

    if (m_watchdogTimer->isActive())
        m_watchdogTimer->stop();

}

void RecognitionProcessor::shutdownAndWait()
{
    if (!m_isProcessing)
        return;

    LogRouter::instance().info(
        "[RecognitionProcessor] shutdownAndWait() invoked.");

    if (m_ocrController)
    {
        m_ocrController->shutdownAndWait();
    }

    finalizeOnce(FinalStatus::Shutdown, tr("Shutdown"));
}


// ============================================================
// Full session reset (Clear button)
// ============================================================
void RecognitionProcessor::clearSession()
{
    // Stage 5 hardening:
    // Session clear is not allowed during processing.
    if (m_isProcessing)
    {
        LogRouter::instance().warning(
            "[RecognitionProcessor] clearSession() ignored: processing active.");
        return;
    }
    LogRouter::instance().info("[RecognitionProcessor] Clearing session");

    // Free allocated LineTables from previous run
    clearOldLineTables();

    // Drop pages snapshot completely
    m_pages.clear();

    // Reset input job configuration.
    // After clear, new jobs must be explicitly provided.
    m_jobs.clear();

    // Reset progress-related counters
    m_lastOcrDone = 0;
}

