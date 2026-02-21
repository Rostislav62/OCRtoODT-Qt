// ============================================================
//  OCRtoODT — Recognition Processor
//  File: src/core/processors/RecognitionProcessor.h
//
//  Responsibility:
//      STEP 2 orchestrator + STEP 3 handoff.
//
//      Coordinates:
//          • STEP 2 (2_ocr): OCR execution → vp.ocrTsvText (RAW TSV, RAM)
//          • STEP 3 (3_LineTextBuilder): RAW TSV → Tsv::LineTable (RAM)
//
//  Output after STEP 3:
//      • QVector<Core::VirtualPage> with vp.lineTable allocated per page
//
//  STRICT RULES:
//      • NO UI widgets
//      • NO OCR implementation here
//      • NO TSV parsing logic here
// ============================================================

#pragma once

#include <QObject>
#include <QVector>
#include <QTimer>

#include "1_preprocess/PageJob.h"

// Forward declarations only (avoid heavy include chains)
namespace Core { struct VirtualPage; }
namespace Ocr  { class OcrPipelineController; }
namespace Core { class ProgressManager; }


class RecognitionProcessor : public QObject
{
    Q_OBJECT

public:
    explicit RecognitionProcessor(QObject *parent = nullptr);
    ~RecognitionProcessor() override;

    // STEP 1 output -> STEP 2 input
    void setJobs(const QVector<Ocr::Preprocess::PageJob> &jobs);

    // Run STEP 2, then automatically run STEP 3
    void run();

    // Pages storage lives inside RecognitionProcessor.
    const QVector<Core::VirtualPage>& pages() const { return m_pages; }
    QVector<Core::VirtualPage>& pagesMutable() { return m_pages; }


    void setProgressManager(Core::ProgressManager *pm);

    void cancel();

    // --------------------------------------------------------
    // Safe shutdown hook (blocking).
    // Used ONLY during application shutdown.
    // --------------------------------------------------------
    void shutdownAndWait();

    // --------------------------------------------------------
    // Full session reset (used by MainWindow::Clear)
    // --------------------------------------------------------
    void clearSession();

    bool isProcessing() const { return m_isProcessing; }

signals:
    // STEP 2 status
    void ocrMessage(const QString &msg);

    // STEP 2 boundary
    void ocrFinished();

    // After STEP 3 completed
    void ocrCompleted(const QVector<Core::VirtualPage> &pages);

    void processingStarted();
    void processingFinished();


private slots:
    void onOcrCompletedFromOcr(const QVector<Core::VirtualPage> &pages);

private:
    enum class FinalStatus
    {
        Success,
        Cancelled,
        Timeout,
        NoJobs,
        Shutdown
    };

    void finalizeOnce(FinalStatus status, const QString &reason);
    void resetFinalizationState();

    bool m_finalized = false;

    // Stage 5 hardening:
    // Explicit run phase eliminates races between ocrFinished and ocrCompleted.
    enum class RunPhase
    {
        Idle,
        Step2_OcrRunning,
        Step3_LineBuilding
    };

    RunPhase m_phase = RunPhase::Idle;

    // Set when we entered Step3 (i.e., ocrCompleted arrived).
    bool m_seenOcrCompleted = false;

    void ensureControllers();
    void clearOldLineTables();

    QVector<Ocr::Preprocess::PageJob> m_jobs;
    QVector<Core::VirtualPage>        m_pages;

    Ocr::OcrPipelineController       *m_ocrController = nullptr;

    int m_lastOcrDone = 0;
    Core::ProgressManager *m_progressManager = nullptr;

    bool m_isProcessing = false;

    QTimer *m_watchdogTimer = nullptr;

};
