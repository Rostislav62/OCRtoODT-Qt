// ============================================================
//  OCRtoODT — OCR Pipeline Worker
//  File: src/2_ocr/OcrPipeLineWorker.h
//
//  Responsibility:
//      STEP 2 — OCR execution worker (RAM-first).
//
//      • Receives preprocessing results (PageJob) from STEP 1
//      • Runs OCR for each page (in parallel)
//      • Works strictly according to global policy:
//            - general.mode
//            - general.debug_mode
//      • Produces OCR results in RAM
//      • Writes TSV to disk ONLY if required by policy
//
//      IMPORTANT:
//          • This class does NOT read ConfigManager.
//          • All policy decisions are injected by Controller.
//          • Disk is NEVER assumed to exist.
//
// ============================================================

#ifndef OCR_PIPELINE_WORKER_H
#define OCR_PIPELINE_WORKER_H

#include <QObject>
#include <QVector>
#include <QFuture>
#include <atomic>

#include "2_ocr/OcrResult.h"

#include "1_preprocess/PageJob.h"
#include "core/VirtualPage.h"

namespace Ocr {

class OcrPipelineWorker : public QObject
{
    Q_OBJECT

public:
    explicit OcrPipelineWorker(QObject *parent = nullptr);

    // --------------------------------------------------------
    // Start OCR pipeline (called via invokeMethod, queued)
    //
    // Parameters:
    //   jobs       — preprocessing results (STEP 1)
    //   mode       — "ram_only" | "disk_only"
    //   debugMode  — global debug switch
    // --------------------------------------------------------
    void start(const QVector<Ocr::Preprocess::PageJob> &jobs,
               const QString &mode,
               bool debugMode);

public slots:
    // --------------------------------------------------------
    // Stop OCR pipeline (runs in worker thread)
    // --------------------------------------------------------
    void cancel();

signals:
    // --------------------------------------------------------
    // OCR STAGE SIGNALS
    // --------------------------------------------------------
    void ocrMessage(QString msg);

    void ocrFinished();

    // Final OCR result:
    // VirtualPages enriched with OCR data
    void ocrCompleted(const QVector<Core::VirtualPage> &pages);


    void ocrProgress(int done, int total);

private:
    QVector<Ocr::Preprocess::PageJob> m_jobs;

    QString m_mode;
    bool    m_debugMode = false;

    // Cancel flag (cooperative)
    std::atomic_bool m_cancelRequested{false};

    // Keep future so cancel() can call m_future.cancel()
    QFuture<OcrPageResult> m_future;
};

} // namespace Ocr

#endif // OCR_PIPELINE_WORKER_H
