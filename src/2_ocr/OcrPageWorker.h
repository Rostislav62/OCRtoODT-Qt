// ============================================================
//  OCRtoODT — OCR Page Worker
//  File: src/2_ocr/OcrPageWorker.h
//
//  Responsibility:
//      Execute OCR for ONE page using preprocessing output.
//
//  CONTRACT RULES:
//      • Input image must be obtained strictly from PageJob:
//          - if keepInRam: enhancedMat must be valid Gray8
//          - else: enhancedPath must point to Gray8 image on disk
//      • Language selection is NOT read from ConfigManager.
//        It is injected by caller as "eng+rus" etc.
//      • Cancellation is cooperative via cancelFlag.
//
// ============================================================

#pragma once

#include <QString>
#include <atomic>

#include "1_preprocess/PageJob.h"
#include "2_ocr/OcrResult.h"

namespace Ocr {

class OcrPageWorker
{
public:
    // --------------------------------------------------------
    // Disk output path for legacy TSV caching (debug / compatibility)
    // --------------------------------------------------------
    static QString buildTsvPath(int globalIndex);

    // --------------------------------------------------------
    // Convenience wrapper: no cancellation
    // --------------------------------------------------------
    static OcrPageResult run(const Ocr::Preprocess::PageJob &job,
                             const QString &languageString);

    // --------------------------------------------------------
    // Main worker entry (cooperative cancel)
    //
    // languageString:
    //   Tesseract format: "eng+rus"
    // cancelFlag:
    //   owned by Controller; may be null
    // --------------------------------------------------------
    static OcrPageResult run(const Ocr::Preprocess::PageJob &job,
                             const QString &languageString,
                             const std::atomic_bool *cancelFlag);
};

} // namespace Ocr
