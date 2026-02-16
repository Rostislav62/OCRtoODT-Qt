// ============================================================
//  OCRtoODT — Preprocess Filters: Shadow Removal
//  File: 1_preprocess/filters/shadow_removal.h
//
//  Responsibility:
//      Reduce strong shadows and illumination gradients,
//      typically present in mobile phone photos.
//
//  Notes for OCR quality:
//      - This filter is powerful but dangerous.
//      - Large kernel sizes may erase thin glyphs
//        (serifs, dots, commas, accents).
//      - Should usually be DISABLED for scanner images.
//
//  Configuration (config.yaml):
//      preprocess.profiles.<profile>.shadow_removal.enabled
//      preprocess.profiles.<profile>.shadow_removal.morph_kernel
//
// ============================================================

#ifndef PREPROCESS_FILTERS_SHADOW_REMOVAL_H
#define PREPROCESS_FILTERS_SHADOW_REMOVAL_H

#include <opencv2/core.hpp>

namespace Ocr {
namespace Preprocess {
namespace Filters {

// ------------------------------------------------------------
// removeShadows
//
// Parameters:
//   src          - input grayscale image (CV_8UC1)
//   morphKernel  - morphology kernel size (odd number)
//
// Recommended ranges:
//   morphKernel : 15 – 101 (odd)
//
// Returns:
//   New grayscale image with reduced shadows.
// ------------------------------------------------------------
cv::Mat removeShadows(const cv::Mat &src,
                      int morphKernel);

} // namespace Filters
} // namespace Preprocess
} // namespace Ocr

#endif // PREPROCESS_FILTERS_SHADOW_REMOVAL_H
