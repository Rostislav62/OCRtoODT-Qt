// ============================================================
//  OCRtoODT — OCR Pipeline Controller
//  File: src/2_ocr/OcrPipelineController.cpp
//
//  Responsibility:
//      Controls full lifecycle of OCR worker.
//
//  Guarantees:
//      • Runtime policy is evaluated BEFORE each RUN
//      • Runtime policy is NEVER changed mid-OCR
//      • Deferred runtime policy is applied AFTER OCR finishes
//      • Safe shutdown (no dangling QtConcurrent jobs)
//      • Deterministic state transitions
//
//  Stage 4.4 (Memory Pressure Runtime B):
//      RuntimePolicyManager::requestReapply()
//      RuntimePolicyManager::onPipelineBecameIdle()
//
// ============================================================

#include "2_ocr/OcrPipeLineController.h"

#include <QThread>      // for QThread::currentThread() in shutdownAndWait()
#include <QMetaObject>  // for invokeMethod()

#include "core/ConfigManager.h"
#include "core/LogRouter.h"
#include "core/RuntimePolicyManager.h"
#include "core/ocr/OcrLanguageManager.h"

using namespace Ocr;

// ------------------------------------------------------------
// Static singleton instance
// ------------------------------------------------------------
OcrPipelineController* OcrPipelineController::s_instance = nullptr;

// ============================================================
// Constructor
// ============================================================
OcrPipelineController::OcrPipelineController(QObject *parent)
    : QObject(parent)
{
    // --------------------------------------------------------
    // Stage 5 hardening:
    // Do not overwrite singleton if already created.
    // --------------------------------------------------------
    if (!s_instance)
    {
        s_instance = this;
    }
    else if (s_instance != this)
    {
        LogRouter::instance().error(
            "[OcrPipelineController] Duplicate instance detected! Singleton not overwritten.");
    }

    // --------------------------------------------------------
    // Worker creation
    // --------------------------------------------------------
    m_worker = new OcrPipelineWorker(this);

    // --------------------------------------------------------
    // Signal forwarding (worker → controller → outer layers)
    // --------------------------------------------------------
    connect(m_worker, &OcrPipelineWorker::ocrMessage,
            this, &OcrPipelineController::ocrMessage);

    connect(m_worker, &OcrPipelineWorker::ocrFinished,
            this, &OcrPipelineController::ocrFinished);

    connect(m_worker, &OcrPipelineWorker::ocrCompleted,
            this, &OcrPipelineController::ocrCompleted);

    connect(m_worker, &OcrPipelineWorker::ocrProgress,
            this, &OcrPipelineController::ocrProgress);

    // --------------------------------------------------------
    // OCR FINISHED → pipeline becomes idle
    // --------------------------------------------------------
    connect(m_worker,
            &OcrPipelineWorker::ocrFinished,
            this,
            [this]()
            {
                LogRouter::instance().info(
                    QString("[STATE] run=%1 CTRL event=WORKER_FINISHED_SIGNAL")
                        .arg(m_runId));

                m_isRunning.store(false);

                LogRouter::instance().info(
                    "[OcrPipelineController] OCR finished -> pipeline idle.");

                // Stage 5 hardening: exactly-once idle notify
                notifyIdleOnce();
            });

    LogRouter::instance().info(
        "[OcrPipelineController] Controller constructed.");
}

// ============================================================
// Destructor
// ============================================================
OcrPipelineController::~OcrPipelineController()
{
    // --------------------------------------------------------
    // Ensure worker is not left running during controller destruction.
    // --------------------------------------------------------
    if (m_isRunning.load())
    {
        LogRouter::instance().warning(
            "[OcrPipelineController] Destructor invoked while running. Forcing shutdown.");

        m_cancelRequested.store(true);
        shutdownAndWait();
    }

    s_instance = nullptr;

    LogRouter::instance().info(
        "[OcrPipelineController] Controller destroyed.");
}

// ============================================================
// Singleton accessor
// ============================================================
OcrPipelineController* OcrPipelineController::instance()
{
    return s_instance;
}

// ============================================================
// EXACTLY-ONCE idle notification barrier
// ============================================================
void OcrPipelineController::notifyIdleOnce()
{
    bool expected = false;
    if (!m_idleNotified.compare_exchange_strong(expected, true))
        return;

    LogRouter::instance().info(
        "[OcrPipelineController] pipeline idle notified (exactly-once).");

    RuntimePolicyManager::onPipelineBecameIdle();
}

// ============================================================
// Start OCR pipeline
// ============================================================
void OcrPipelineController::start(
    const QVector<Ocr::Preprocess::PageJob> &jobs)
{
    if (m_isRunning.load())
    {
        LogRouter::instance().warning(
            "[OcrPipelineController] start() called while already running. Ignored.");
        return;
    }

    // --------------------------------------------------------
    // Reset idle notification barrier for this run.
    // --------------------------------------------------------
    m_idleNotified.store(false);

    if (jobs.isEmpty())
    {
        LogRouter::instance().warning(
            "[OcrPipelineController] start() ignored: jobs is empty.");
        return;
    }

    // --------------------------------------------------------
    // Re-evaluate runtime policy BEFORE RUN.
    // --------------------------------------------------------
    RuntimePolicyManager::requestReapply(false);

    ConfigManager &cfg = ConfigManager::instance();

    const QString mode =
        cfg.get("general.mode", "ram_only").toString();

    const bool debugMode =
        cfg.get("general.debug_mode", false).toBool();

    // --------------------------------------------------------
    // Resolve language string ONCE per RUN (RUN invariant)
    // --------------------------------------------------------
    const QString languageString =
        OcrLanguageManager::instance()
            .buildTesseractLanguageString();

    LogRouter::instance().info(
        QString("[OcrPipelineController] Starting OCR (jobs=%1, mode=%2, debug=%3, lang=%4)")
            .arg(jobs.size())
            .arg(mode)
            .arg(debugMode ? "true" : "false")
            .arg(languageString));

    m_cancelRequested.store(false);
    m_isRunning.store(true);

    LogRouter::instance().info(
        QString("[STATE] run=%1 CTRL event=INVOKE_WORKER_START jobs=%2")
            .arg(m_runId)
            .arg(jobs.size()));

    // --------------------------------------------------------
    // Asynchronous start in worker thread
    // --------------------------------------------------------
    QMetaObject::invokeMethod(
        m_worker,
        [this, jobs, mode, debugMode, languageString]()
        {
            // --------------------------------------------------------
            // Trace correlation: propagate run id into worker before start
            // --------------------------------------------------------
            m_worker->setRunId(m_runId);

            m_worker->start(
                jobs,
                mode,
                debugMode,
                languageString,
                &m_cancelRequested);
        },
        Qt::QueuedConnection);
}

// ============================================================
// Cancel OCR pipeline
// ============================================================
void OcrPipelineController::cancel()
{
    LogRouter::instance().info(
        QString("[STATE] run=%1 CTRL event=CANCEL_ENTER isRunning=%2")
            .arg(m_runId)
            .arg(m_isRunning.load()));

    if (!m_isRunning.load())
        return;

    LogRouter::instance().warning(
        "[OcrPipelineController] Cancel requested.");

    m_cancelRequested.store(true);

    shutdownAndWait();
}

// ============================================================
// Running state
// ============================================================
bool OcrPipelineController::isRunning() const
{
    return m_isRunning.load();
}

// ============================================================
// Safe shutdown
// ============================================================
void OcrPipelineController::shutdownAndWait()
{
    LogRouter::instance().info(
        QString("[STATE] run=%1 CTRL event=SHUTDOWN_BEGIN")
            .arg(m_runId));

    if (!m_worker)
        return;

    // --------------------------------------------------------
    // Even if already not running — guarantee deterministic idle notification.
    // --------------------------------------------------------
    if (!m_isRunning.load())
    {
        notifyIdleOnce();
        return;
    }

    // --------------------------------------------------------
    // Wait for worker future to finish safely
    // --------------------------------------------------------
    if (m_worker->thread() == QThread::currentThread())
    {
        m_worker->waitForFinished();
    }
    else
    {
        QMetaObject::invokeMethod(
            m_worker,
            &OcrPipelineWorker::waitForFinished,
            Qt::BlockingQueuedConnection);
    }

    m_isRunning.store(false);

    LogRouter::instance().info(
        "[OcrPipelineController] shutdown complete -> pipeline idle.");

    notifyIdleOnce();

    LogRouter::instance().info(
        QString("[STATE] run=%1 CTRL event=SHUTDOWN_DONE")
            .arg(m_runId));
}
