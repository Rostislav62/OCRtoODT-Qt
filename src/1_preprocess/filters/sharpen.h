// ============================================================
//  OCRtoODT — Preprocess Filters: Sharpen (Unsharp Mask)
//  File: 1_preprocess/filters/sharpen.h
//
//  Responsibility:
//      Apply controlled sharpening using unsharp mask technique.
//
//  Notes for OCR quality (CRITICAL):
//      - Sharpening is one of the MOST DANGEROUS operations for OCR
//      - Even small values may destroy thin strokes and punctuation
//      - Recommended to keep DISABLED for most documents
//
//  Configuration (config.yaml):
//      preprocess.profiles.<profile>.sharpen.enabled
//      preprocess.profiles.<profile>.sharpen.strength
//      preprocess.profiles.<profile>.sharpen.gaussian_ksize
//      preprocess.profiles.<profile>.sharpen.gaussian_sigma
//
// ============================================================

#ifndef PREPROCESS_FILTERS_SHARPEN_H
#define PREPROCESS_FILTERS_SHARPEN_H

#include <opencv2/core.hpp>

namespace Ocr {
namespace Preprocess {
namespace Filters {

// ------------------------------------------------------------
// unsharpMask
//
// Parameters:
//   src            - input grayscale image (CV_8UC1)
//   strength       - sharpening strength multiplier
//   gaussianKSize  - kernel size for internal Gaussian blur (odd)
//   gaussianSigma  - sigma for internal Gaussian blur
//
// Recommended ranges:
//   strength      : 0.0 – 2.0  (0.0 disables effect)
//   gaussianKSize : 3 – 21 (odd)
//   gaussianSigma : 0.1 – 5.0
//
// Returns:
//   Sharpened grayscale image.
// ------------------------------------------------------------
cv::Mat unsharpMask(const cv::Mat &src,
                    double strength,
                    int gaussianKSize,
                    double gaussianSigma);

} // namespace Filters
} // namespace Preprocess
} // namespace Ocr

#endif // PREPROCESS_FILTERS_SHARPEN_H
