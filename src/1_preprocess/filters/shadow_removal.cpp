// ============================================================
//  OCRtoODT — Preprocess Filters: Shadow Removal
//  File: 1_preprocess/filters/shadow_removal.cpp
//
//  Implementation details:
//      - Uses morphological opening to estimate background
//      - Subtracts background to flatten illumination
//      - Normalizes result back to 8-bit range
//
//  Algorithm outline:
//      1) Estimate background using large morphological kernel
//      2) Compute absolute difference from original image
//      3) Normalize intensity to full [0..255] range
//
//  IMPORTANT:
//      This filter MUST operate on grayscale images only.
// ============================================================

#include "1_preprocess/filters/shadow_removal.h"

#include <opencv2/imgproc.hpp>

namespace Ocr {
namespace Preprocess {
namespace Filters {

cv::Mat removeShadows(const cv::Mat &src,
                      int morphKernel)
{
    // Defensive checks
    if (src.empty())
        return cv::Mat();

    if (src.channels() != 1)
        return src.clone();

    // Ensure kernel size is valid
    if (morphKernel < 3)
        morphKernel = 3;
    if ((morphKernel % 2) == 0)
        morphKernel += 1;

    // --------------------------------------------------------
    // 1) Estimate background via morphological opening
    // --------------------------------------------------------
    cv::Mat background;
    cv::Mat kernel =
        cv::getStructuringElement(cv::MORPH_RECT,
                                  cv::Size(morphKernel, morphKernel));

    cv::morphologyEx(src,
                     background,
                     cv::MORPH_OPEN,
                     kernel);

    // --------------------------------------------------------
    // 2) Remove background (flatten illumination)
    // --------------------------------------------------------
    cv::Mat diff;
    cv::absdiff(src, background, diff);

    // --------------------------------------------------------
    // 3) Normalize to full grayscale range
    // --------------------------------------------------------
    cv::Mat normalized;
    cv::normalize(diff,
                  normalized,
                  0,
                  255,
                  cv::NORM_MINMAX,
                  CV_8UC1);

    return normalized;
}

} // namespace Filters
} // namespace Preprocess
} // namespace Ocr
