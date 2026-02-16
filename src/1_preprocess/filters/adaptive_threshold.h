// ============================================================
//  OCRtoODT — Preprocess Filters: Adaptive Threshold
//  File: 1_preprocess/filters/adaptive_threshold.h
//
//  Responsibility:
//      Perform adaptive (local) thresholding to produce
//      a binary image (black/white).
//
//  Notes for OCR quality (VERY IMPORTANT):
//      - Tesseract works BEST with grayscale images
//      - Binarization often introduces false punctuation:
//            dots, commas, quotes, noise
//      - This filter should be DISABLED by default
//
//  Use cases:
//      - Extremely low-contrast text
//      - Heavily degraded scans
//      - Special experiments only
//
//  Configuration (config.yaml):
//      preprocess.profiles.<profile>.adaptive_threshold.enabled
//      preprocess.profiles.<profile>.adaptive_threshold.block_size
//      preprocess.profiles.<profile>.adaptive_threshold.C
//
// ============================================================

#ifndef PREPROCESS_FILTERS_ADAPTIVE_THRESHOLD_H
#define PREPROCESS_FILTERS_ADAPTIVE_THRESHOLD_H

#include <opencv2/core.hpp>

namespace Ocr {
namespace Preprocess {
namespace Filters {

// ------------------------------------------------------------
// adaptiveThreshold
//
// Parameters:
//   src       - input grayscale image (CV_8UC1)
//   blockSize - size of pixel neighborhood (odd)
//   C         - constant subtracted from local mean
//
// Recommended ranges:
//   blockSize : 11 – 101 (odd)
//   C         : -20 – 20
//
// Returns:
//   Binary image (CV_8UC1, values 0 or 255).
// ------------------------------------------------------------
cv::Mat adaptiveThreshold(const cv::Mat &src,
                          int blockSize,
                          int C);

} // namespace Filters
} // namespace Preprocess
} // namespace Ocr

#endif // PREPROCESS_FILTERS_ADAPTIVE_THRESHOLD_H
