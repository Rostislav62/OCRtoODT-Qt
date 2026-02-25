// ============================================================
//  OCRtoODT — Recognition Processor
//  File: src/core/processors/RecognitionProcessor.cpp
// ============================================================

#include "core/processors/RecognitionProcessor.h"
#include <atomic>
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
// State machine helpers
// ============================================================

const char* RecognitionProcessor::stateToStr(PipelineState st)
{
    switch (st)
    {
    case PipelineState::Idle:                return "IDLE";
    case PipelineState::Step1_Preprocess:    return "STEP1_PREPROCESS";
    case PipelineState::Step2_OcrRunning:    return "STEP2_OCR_RUNNING";
    case PipelineState::Step2_CancelRequested:return "STEP2_CANCEL_REQUESTED";
    case PipelineState::Step2_ShuttingDown:  return "STEP2_SHUTTING_DOWN";
    case PipelineState::Step3_LineBuilding:  return "STEP3_LINE_BUILDING";
    case PipelineState::Step3_CancelRequested:return "STEP3_CANCEL_REQUESTED";
    case PipelineState::Completed:           return "COMPLETED";
    }
    return "UNKNOWN";
}

void RecognitionProcessor::traceState(const char *event, const QString &details)
{
    const uint64_t seq = ++m_seq;

    QString msg = QString("[STATE] run=%1 seq=%2 state=%3 event=%4")
                      .arg(m_runId)
                      .arg(seq)
                      .arg(stateToStr(m_state))
                      .arg(event);

    if (!details.isEmpty())
        msg += QString(" %1").arg(details);

    LogRouter::instance().info(msg);
}

void RecognitionProcessor::setState(PipelineState st, const char *event)
{
    // Always trace transitions, even if state remains the same.
    const PipelineState prev = m_state;
    m_state = st;

    traceState(event,
               QString("prev=%1 next=%2")
                   .arg(stateToStr(prev))
                   .arg(stateToStr(st)));
}


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

    // --------------------------------------------------------
    // CANCEL finalization hook
    // --------------------------------------------------------
    connect(m_ocrController,
            &Ocr::OcrPipelineController::ocrFinished,
            this,
            [this]()
            {
                // If STEP2 was cancelled — finalize here.
                if (m_state == PipelineState::Step2_CancelRequested ||
                    m_state == PipelineState::Step2_ShuttingDown)
                {
                    traceState("CANCEL_FINALIZE_ON_OCR_FINISHED");

                    setState(PipelineState::Idle, "CANCEL_TO_IDLE");

                    finalizeOnce(FinalStatus::Cancelled, tr("Cancelled"));
                }
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

void RecognitionProcessor::resetFinalizationState()
{
    m_finalized = false;

    // Stage 5 hardening:
    // Reset run-phase markers for the new run.
    m_phase = RunPhase::Idle;
    m_seenOcrCompleted = false;

    // State machine reset:
    // Sequence is monotonic across process lifetime; only run_id increments.
    m_state = PipelineState::Idle;

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

        traceState("SETJOBS_IGNORED_PROCESSING_ACTIVE");

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

    resetFinalizationState(); // сначала сброс состояния

    // New run id for deterministic tracing
    ++m_runId;
    traceState("RUN_REQUESTED");


    // Defensive guard:
    // Running without jobs is invalid state unless explicitly reconfigured.
    // Prevent accidental re-run with stale state.
    if (m_jobs.isEmpty())
    {
        LogRouter::instance().warning(
            "[RecognitionProcessor] run() ignored: no jobs configured.");
        return;
    }


    setState(PipelineState::Step2_OcrRunning, "ENTER_STEP2");


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

    traceState("CALL_CONTROLLER_START",
               QString("jobs=%1 timeoutSec=%2").arg(m_jobs.size()).arg(timeoutSec));

    // Investigation mode:
    // Force single-thread externally (controller/pipeline reads config).
    const bool forceSingleThread =
        ConfigManager::instance().get("general.force_single_thread", false).toBool();

    if (forceSingleThread)
        traceState("FORCE_SINGLE_THREAD_ENABLED");

    if (m_ocrController)
        m_ocrController->setRunId(m_runId);

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
        traceState("RECEIVED_OCR_COMPLETED_SIGNAL",
                   QString("pages=%1").arg(pages.size()));

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

    // If cancel was requested, completion must not advance into STEP3.
    if (m_state == PipelineState::Step2_CancelRequested ||
        m_state == PipelineState::Step2_ShuttingDown ||
        m_state == PipelineState::Step3_CancelRequested)
    {
        traceState("IGNORED_COMPLETED_DUE_TO_CANCEL");
        return;
    }

    setState(PipelineState::Step3_LineBuilding, "ENTER_STEP3");

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


    traceState("STEP3_LOOP_BEGIN",
               QString("pages=%1 mode=%2 debug=%3")
                   .arg(m_pages.size())
                   .arg(mode)
                   .arg(debugMode));




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

    setState(PipelineState::Completed, "PIPELINE_COMPLETED");

    // Stage 5 hardening:
    // Finalize FIRST (flip flags, finish progress), then publish results.
    finalizeOnce(FinalStatus::Success, tr("OCR completed"));

    // After finalize barrier, we publish the result snapshot.
    traceState("EMIT_OCR_COMPLETED",
               QString("pages=%1 withLineTable=%2").arg(m_pages.size()).arg(withTable));

    emit ocrCompleted(m_pages);
}

void RecognitionProcessor::cancel()
{
    if (!m_isProcessing)
        return;

    // Prevent repeated cancel from spamming logs and re-entering shutdown.
    // Cancel must be strictly idempotent.
    //
    // Ignore if:
    //  • already idle
    //  • already shutting down
    //  • cancel already requested
    //  • already completed
    //
    if (m_state == PipelineState::Idle ||
        m_state == PipelineState::Step2_ShuttingDown ||
        m_state == PipelineState::Step2_CancelRequested ||
        m_state == PipelineState::Step3_CancelRequested ||
        m_state == PipelineState::Completed)
    {
        traceState("CANCEL_IGNORED_ALREADY_IN_TERMINAL_STATE");
        return;
    }

    setState(PipelineState::Step2_CancelRequested, "UI_CANCEL_REQUESTED");

    LogRouter::instance().info(
        "[RecognitionProcessor] Cancel requested.");


    traceState("CALL_CONTROLLER_CANCEL");

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

    setState(PipelineState::Step2_ShuttingDown, "SHUTDOWN_AND_WAIT_ENTER");

    LogRouter::instance().info(
        "[RecognitionProcessor] shutdownAndWait() invoked.");

    if (m_ocrController)
    {
        traceState("CALL_CONTROLLER_SHUTDOWN_AND_WAIT");

        m_ocrController->shutdownAndWait();
    }

    setState(PipelineState::Idle, "SHUTDOWN_AND_WAIT_DONE");

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

        traceState("CLEARSESSION_IGNORED_PROCESSING_ACTIVE");

        return;
    }
    LogRouter::instance().info("[RecognitionProcessor] Clearing session");

    setState(PipelineState::Idle, "CLEARSESSION");

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

