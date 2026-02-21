// ============================================================
//  OCRtoODT — OCR Pipeline Controller
//  File: src/2_ocr/OcrPipelineController.h
//
//  Responsibility:
//      STEP 2 — OCR (Text Recognition)
//
//      High-level orchestrator controlling OCR pipeline:
//          • Owns and manages OCR worker thread
//          • Starts OCR asynchronously
//          • Applies runtime policy before each RUN
//          • Exposes running state
//
//      Runtime Policy Integration:
//          • RuntimePolicyManager::reapply() is called:
//                - before each RUN
//                - after safe shutdown
//
//      Singleton Access:
//          • instance() provides global access for
//            SettingsDialog safety checks.
//          • Exactly one controller exists per application.
//
// ============================================================

#ifndef OCR_PIPELINE_CONTROLLER_H
#define OCR_PIPELINE_CONTROLLER_H

#include <QObject>
#include <QVector>
#include <atomic>

#include "core/VirtualPage.h"
#include "1_preprocess/PageJob.h"
#include "2_ocr/OcrPipeLineWorker.h"

namespace Ocr {

class OcrPipelineController : public QObject
{
    Q_OBJECT

public:
    explicit OcrPipelineController(QObject *parent = nullptr);
    ~OcrPipelineController() override;

    // --------------------------------------------------------
    // Singleton accessor
    // --------------------------------------------------------
    static OcrPipelineController* instance();

    // --------------------------------------------------------
    // Start OCR pipeline (asynchronous)
    // --------------------------------------------------------
    void start(const QVector<Ocr::Preprocess::PageJob> &jobs);

    void cancel();

    // --------------------------------------------------------
    // Safe shutdown hooks
    // --------------------------------------------------------
    bool isRunning() const;
    void shutdownAndWait();

signals:
    void ocrMessage(QString msg);
    void ocrFinished();
    void ocrCompleted(const QVector<Core::VirtualPage> &pages);
    void ocrProgress(int done, int total);

private:
    static OcrPipelineController* s_instance;

    OcrPipelineWorker *m_worker = nullptr;

    std::atomic_bool m_cancelRequested{false};
    std::atomic_bool m_isRunning{false};

    // Stage 5 hardening:
    // Idle notification must be EXACTLY-ONCE per run/shutdown path,
    // otherwise RuntimePolicyManager may reapply policy twice.
    std::atomic_bool m_idleNotified{false};

    void notifyIdleOnce();
};

} // namespace Ocr

#endif // OCR_PIPELINE_CONTROLLER_H
