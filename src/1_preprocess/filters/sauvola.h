// ============================================================
//  OCRtoODT — Preprocess Filters: Sauvola Binarization
//  File: 1_preprocess/filters/sauvola.h
//
//  Responsibility:
//      Perform Sauvola adaptive binarization.
//
//  Notes for OCR quality (IMPORTANT):
//      - Sauvola is more conservative than simple adaptive threshold
//      - Still DESTROYS grayscale information
//      - Should be used only for severely degraded documents
//
//  Configuration (config.yaml):
//      preprocess.profiles.<profile>.sauvola.enabled
//      preprocess.profiles.<profile>.sauvola.window_size
//      preprocess.profiles.<profile>.sauvola.k
//      preprocess.profiles.<profile>.sauvola.R
//
// ============================================================

#ifndef PREPROCESS_FILTERS_SAUVOLA_H
#define PREPROCESS_FILTERS_SAUVOLA_H

#include <opencv2/core.hpp>

namespace Ocr {
namespace Preprocess {
namespace Filters {

// ------------------------------------------------------------
// sauvolaBinarize
//
// Parameters:
//   src        - input grayscale image (CV_8UC1)
//   windowSize - local window size (odd)
//   k          - Sauvola sensitivity parameter
//   R          - dynamic range of standard deviation
//
// Recommended ranges:
//   windowSize : 15 – 101 (odd)
//   k          : 0.1 – 0.6
//   R          : 64 – 128
//
// Returns:
//   Binary image (CV_8UC1, values 0 or 255).
// ------------------------------------------------------------
cv::Mat sauvolaBinarize(const cv::Mat &src,
                        int windowSize,
                        double k,
                        double R);

} // namespace Filters
} // namespace Preprocess
} // namespace Ocr

#endif // PREPROCESS_FILTERS_SAUVOLA_H
