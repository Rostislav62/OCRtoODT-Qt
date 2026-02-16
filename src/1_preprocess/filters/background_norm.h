// ============================================================
//  OCRtoODT — Preprocess Filters: Background Normalization
//  File: 1_preprocess/filters/background_norm.h
//
//  Responsibility:
//      Normalize uneven background illumination by estimating
//      and compensating low-frequency background gradients.
//
//  Notes for OCR quality:
//      - Useful for mobile photos and uneven lighting
//      - Can DAMAGE thin strokes if overused
//      - Should usually be disabled for clean scanner images
//
//  Configuration (config.yaml):
//      preprocess.profiles.<profile>.background_normalization.enabled
//      preprocess.profiles.<profile>.background_normalization.blur_ksize
//      preprocess.profiles.<profile>.background_normalization.epsilon
//
// ============================================================

#ifndef PREPROCESS_FILTERS_BACKGROUND_NORM_H
#define PREPROCESS_FILTERS_BACKGROUND_NORM_H

#include <opencv2/core.hpp>

namespace Ocr {
namespace Preprocess {
namespace Filters {

// ------------------------------------------------------------
// normalizeBackground
//
// Parameters:
//   src        - input grayscale image (CV_8UC1)
//   blurKSize  - kernel size for background estimation (odd)
//   epsilon    - small constant to avoid division by zero
//
// Recommended ranges:
//   blurKSize : 15 – 201 (odd)
//   epsilon   : 0.0001 – 1.0
//
// Returns:
//   Grayscale image with normalized background.
// ------------------------------------------------------------
cv::Mat normalizeBackground(const cv::Mat &src,
                            int blurKSize,
                            double epsilon);

} // namespace Filters
} // namespace Preprocess
} // namespace Ocr

#endif // PREPROCESS_FILTERS_BACKGROUND_NORM_H
