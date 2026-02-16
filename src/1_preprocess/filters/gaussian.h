// ============================================================
//  OCRtoODT — Preprocess Filters: Gaussian Blur
//  File: 1_preprocess/filters/gaussian.h
//
//  Responsibility:
//      Apply Gaussian blur for noise reduction before OCR.
//
//  Notes for OCR quality:
//      - Small blur helps suppress scanner noise and JPEG artifacts
//      - Large blur destroys thin strokes (i, l, punctuation)
//      - Should usually be VERY WEAK or disabled for clean scans
//
//  Configuration (config.yaml):
//      preprocess.profiles.<profile>.gaussian_blur.enabled
//      preprocess.profiles.<profile>.gaussian_blur.kernel_size
//      preprocess.profiles.<profile>.gaussian_blur.sigma
//
// ============================================================

#ifndef PREPROCESS_FILTERS_GAUSSIAN_H
#define PREPROCESS_FILTERS_GAUSSIAN_H

#include <opencv2/core.hpp>

namespace Ocr {
namespace Preprocess {
namespace Filters {

// ------------------------------------------------------------
// gaussianBlur
//
// Parameters:
//   src        - input grayscale image (CV_8UC1)
//   kernelSize - Gaussian kernel size (odd number)
//   sigma      - Gaussian sigma value
//
// Recommended ranges:
//   kernelSize : 3 – 21 (odd)
//   sigma      : 0.1 – 5.0
//
// Returns:
//   Blurred grayscale image.
// ------------------------------------------------------------
cv::Mat gaussianBlur(const cv::Mat &src,
                     int kernelSize,
                     double sigma);

} // namespace Filters
} // namespace Preprocess
} // namespace Ocr

#endif // PREPROCESS_FILTERS_GAUSSIAN_H
