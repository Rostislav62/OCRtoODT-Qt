// ============================================================
//  OCRtoODT — OCR Pipeline Controller
//  File: src/2_ocr/OcrPipelineController.cpp
//
//  Controls OCR worker thread:
//      • creates and owns worker
//      • applies global OCR policy
//      • forwards OCR signals
//      • invokes OCR asynchronously
//
// ============================================================

#include "2_ocr/OcrPipeLineController.h"

#include "core/ConfigManager.h"
#include "core/LogRouter.h"

using namespace Ocr;

// ============================================================
// Constructor
// ============================================================
OcrPipelineController::OcrPipelineController(QObject *parent)
    : QObject(parent)
{
    m_worker = new OcrPipelineWorker();


    // --------------------------------------------------------
    // Forward OCR signals from worker
    // --------------------------------------------------------
    connect(m_worker, &OcrPipelineWorker::ocrMessage,
            this, &OcrPipelineController::ocrMessage);

    connect(m_worker, &OcrPipelineWorker::ocrFinished,
            this, &OcrPipelineController::ocrFinished);

    connect(m_worker, &OcrPipelineWorker::ocrCompleted,
            this, &OcrPipelineController::ocrCompleted);

    connect(m_worker,
            &OcrPipelineWorker::ocrProgress,
            this,
            &OcrPipelineController::ocrProgress);

    LogRouter::instance().info(
        "[OcrPipelineController] OCR thread started.");
}

// ============================================================
// Destructor
// ============================================================
OcrPipelineController::~OcrPipelineController()
{

    LogRouter::instance().info(
        "[OcrPipelineController] OCR thread stopped safely.");
}


// ============================================================
// Start OCR pipeline asynchronously
// ============================================================
void OcrPipelineController::start(
    const QVector<Ocr::Preprocess::PageJob> &jobs)
{
    ConfigManager &cfg = ConfigManager::instance();

    const QString mode =
        cfg.get("general.mode", "ram_only").toString();

    const bool debugMode =
        cfg.get("general.debug_mode", false).toBool();

    LogRouter::instance().info(
        QString("[OcrPipelineController] Starting OCR (jobs=%1, mode=%2, debug=%3)")
            .arg(jobs.size())
            .arg(mode)
            .arg(debugMode));

    // --------------------------------------------------------
    // Forward jobs and policy to worker (queued)
    // --------------------------------------------------------
    QMetaObject::invokeMethod(
        m_worker,
        [this, jobs, mode, debugMode]()
        {
            // STEP 2 — OCR
            m_worker->start(jobs, mode, debugMode);
        },
        Qt::QueuedConnection
        );
}

// ============================================================
// Cancel OCR pipeline (queued)
// ============================================================
void OcrPipelineController::cancel()
{
    if (!m_worker)
        return;

    QMetaObject::invokeMethod(
        m_worker,
        [this]()
        {
            m_worker->cancel();
        },
        Qt::QueuedConnection
        );
}


