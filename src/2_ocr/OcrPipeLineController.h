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
//          • Applies global OCR policy (RAM / DISK)
//          • Forwards OCR-related signals to outer layers
//
//      Input:
//          • QVector<Ocr::Preprocess::PageJob>
//            (results of STEP 1 preprocessing)
//
//      Output:
//          • QVector<Core::VirtualPage> enriched with:
//                - ocrSuccess
//                - OCR result (RAM or disk, policy-driven)
//
//      IMPORTANT:
//          • This controller is the ONLY place where
//            general.mode / general.debug_mode are read for OCR.
//          • OCR workers do NOT read config directly.
//
// ============================================================

#ifndef OCR_PIPELINE_CONTROLLER_H
#define OCR_PIPELINE_CONTROLLER_H

#include <QObject>
#include <QThread>
#include <QVector>

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
    // Start OCR pipeline (asynchronous)
    // --------------------------------------------------------
    void start(const QVector<Ocr::Preprocess::PageJob> &jobs);

    void cancel();


signals:
    // --------------------------------------------------------
    // OCR runtime messages (logging / UI)
    // --------------------------------------------------------
    void ocrMessage(QString msg);

    // --------------------------------------------------------
    // OCR finished (status-only signal)
    // --------------------------------------------------------
    void ocrFinished();

    // --------------------------------------------------------
    // OCR completed (MAIN RESULT)
    //
    // Pages are returned with OCR results attached.
    // --------------------------------------------------------
    void ocrCompleted(const QVector<Core::VirtualPage> &pages);

    void ocrProgress(int done, int total);

private:
    OcrPipelineWorker *m_worker = nullptr;
};

} // namespace Ocr

#endif // OCR_PIPELINE_CONTROLLER_H
