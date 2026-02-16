// ============================================================
//  OCRtoODT — Preprocess Filters: CLAHE
//  File: 1_preprocess/filters/clahe.h
//
//  Responsibility:
//      Apply CLAHE (Contrast Limited Adaptive Histogram Equalization)
//      to improve local contrast in grayscale images.
//
//  Notes for OCR quality:
//      - Can significantly improve faded or low-contrast text
//      - Excessive clipLimit produces background noise
//      - Often harmful for already clean scanner images
//
//  Configuration (config.yaml):
//      preprocess.profiles.<profile>.clahe.enabled
//      preprocess.profiles.<profile>.clahe.clip_limit
//      preprocess.profiles.<profile>.clahe.tile_grid_size
//
// ============================================================

#ifndef PREPROCESS_FILTERS_CLAHE_H
#define PREPROCESS_FILTERS_CLAHE_H

#include <opencv2/core.hpp>

namespace Ocr {
namespace Preprocess {
namespace Filters {

// ------------------------------------------------------------
// applyClahe
//
// Parameters:
//   src           - input grayscale image (CV_8UC1)
//   clipLimit     - threshold for contrast limiting
//   tileGridSize  - size of grid for histogram equalization
//
// Recommended ranges:
//   clipLimit    : 1.0 – 10.0
//   tileGridSize : 4 – 16
//
// Returns:
//   Grayscale image with CLAHE applied.
// ------------------------------------------------------------
cv::Mat applyClahe(const cv::Mat &src,
                   double clipLimit,
                   int tileGridSize);

} // namespace Filters
} // namespace Preprocess
} // namespace Ocr

#endif // PREPROCESS_FILTERS_CLAHE_H
