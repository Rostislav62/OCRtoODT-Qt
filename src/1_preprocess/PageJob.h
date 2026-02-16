// ============================================================
//  OCRtoODT — Preprocess: Page Job Structure
//  File: src/1_preprocess/PageJob.h
//
//  Responsibility:
//      Unified passive data container for storing preprocessing
//      results of a single VirtualPage.
//
//      PageJob MUST NOT contain any policy or decision logic.
//      It is filled by PreprocessPipeline and consumed by
//      subsequent steps (OCR, Preview, Export).
//
//  Pipeline model:
//      Load → Enhance → (optional Disk Save) → PageJob
//
//  IMPORTANT ARCHITECTURAL RULES:
//      • EnhanceProcessor:
//            - produces enhancedMat ONLY (pure RAM transform)
//            - never writes to disk
//
//      • PreprocessPipeline:
//            - is the ONLY place where RAM/DISK policy is applied
//            - sets keepInRam / savedToDisk / enhancedPath
//            - follows general.mode and general.debug_mode
//      IMPORTANT:
//          • PageJob contains DATA ONLY
//          • NO policy
//          • NO decisions
//
// ============================================================

#ifndef PREPROCESS_PAGEJOB_H
#define PREPROCESS_PAGEJOB_H

#include <QString>
#include <QSize>
#include <opencv2/core.hpp>

#include "core/VirtualPage.h"

namespace Ocr {
namespace Preprocess {

struct PageJob
{
    // --------------------------------------------------------
    // Page identity
    // --------------------------------------------------------
    int               globalIndex = 0;
    Core::VirtualPage vp;

    // --------------------------------------------------------
    // Preprocessing result
    // --------------------------------------------------------
    cv::Mat           enhancedMat;      // Valid if keepInRam == true
    QString           enhancedPath;     // Valid if savedToDisk == true

    QString           enhanceProfile;
    bool              wasEnhanced = false;

    QSize             enhancedSize;     // Final image size (pixels)

    // --------------------------------------------------------
    // OCR CONTRACT DATA
    // --------------------------------------------------------
    int               ocrDpi = 300;      // FINAL DPI for OCR (per page)

    // --------------------------------------------------------
    // RAM / Disk policy flags
    // (SET ONLY BY PreprocessPipeline)
    // --------------------------------------------------------
    bool              keepInRam   = true;
    bool              savedToDisk = false;

    qint64            enhancedBytes = 0;
};

} // namespace Preprocess
} // namespace Ocr

#endif // PREPROCESS_PAGEJOB_H
