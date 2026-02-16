// ============================================================
//  OCRtoODT — OCR Page Worker
//  File: src/2_ocr/OcrPageWorker.h
//
//  Responsibility:
//      Perform OCR for a single preprocessed page (STEP 2).
//
//  Input:
//      • Ocr::Preprocess::PageJob (contract-driven)
//
//  Output:
//      • OcrPageResult (success + final TSV path)
//
//  Contract (IMPORTANT):
//      • This worker does NOT decide RAM vs DISK for IMAGE source.
//      • It MUST execute STEP 1 decision:
//            job.keepInRam == true  → use enhancedMat
//            job.keepInRam == false → load grayscale from enhancedPath
//
//  Transitional disk I/O note (current step):
//      • Per-pass TSV temp files are no longer written.
//      • Worker writes only ONE final TSV (best pass).
//      • Full RAM-only TSV handoff will be done next at pipeline level.
// ============================================================

#ifndef OCR_PAGE_WORKER_H
#define OCR_PAGE_WORKER_H

#include "1_preprocess/PageJob.h"
#include "2_ocr/OcrResult.h"
#include <atomic>

namespace Ocr {

class OcrPageWorker
{
public:
    static OcrPageResult run(const Ocr::Preprocess::PageJob &job);

    static OcrPageResult run(const Ocr::Preprocess::PageJob &job,
                             const std::atomic_bool *cancelFlag);

private:
    static QString buildTsvPath(int globalIndex);
};

} // namespace Ocr

#endif // OCR_PAGE_WORKER_H
