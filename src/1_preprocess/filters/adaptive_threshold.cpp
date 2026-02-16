// ============================================================
//  OCRtoODT — Preprocess Filters: Adaptive Threshold
//  File: 1_preprocess/filters/adaptive_threshold.cpp
//
//  Implementation details:
//      - Uses cv::adaptiveThreshold (Gaussian method)
//      - Produces a strictly binary image
//      - Operates ONLY on grayscale input
//
//  WARNING:
//      This filter discards grayscale information permanently.
// ============================================================

#include "1_preprocess/filters/adaptive_threshold.h"

#include <opencv2/imgproc.hpp>

namespace Ocr {
namespace Preprocess {
namespace Filters {

cv::Mat adaptiveThreshold(const cv::Mat &src,
                          int blockSize,
                          int C)
{
    // Defensive checks
    if (src.empty())
        return cv::Mat();

    if (src.channels() != 1)
        return src.clone();

    // Sanitize parameters
    if (blockSize < 3)
        blockSize = 3;
    if ((blockSize % 2) == 0)
        blockSize += 1;

    if (blockSize < 11)
        blockSize = 11;

    cv::Mat dst;
    cv::adaptiveThreshold(src,
                          dst,
                          255,
                          cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                          cv::THRESH_BINARY,
                          blockSize,
                          C);

    return dst;
}

} // namespace Filters
} // namespace Preprocess
} // namespace Ocr
