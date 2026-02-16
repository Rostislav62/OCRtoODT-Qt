// ============================================================
//  OCRtoODT — Preprocess Filters: Background Normalization
//  File: 1_preprocess/filters/background_norm.cpp
//
//  Implementation details:
//      - Background is estimated using heavy Gaussian blur
//      - Image is normalized by background division
//      - Output is rescaled to 8-bit grayscale
//
//  WARNING:
//      Excessive blur size may erase fine textual details.
// ============================================================

#include "1_preprocess/filters/background_norm.h"

#include <opencv2/imgproc.hpp>

namespace Ocr {
namespace Preprocess {
namespace Filters {

cv::Mat normalizeBackground(const cv::Mat &src,
                            int blurKSize,
                            double epsilon)
{
    // Defensive checks
    if (src.empty())
        return cv::Mat();

    if (src.channels() != 1)
        return src.clone();

    // Sanitize parameters
    if (blurKSize < 3)
        blurKSize = 3;
    if ((blurKSize % 2) == 0)
        blurKSize += 1;

    if (epsilon <= 0.0)
        epsilon = 0.001;

    // --------------------------------------------------------
    // 1) Estimate background via heavy Gaussian blur
    // --------------------------------------------------------
    cv::Mat background;
    cv::GaussianBlur(src,
                     background,
                     cv::Size(blurKSize, blurKSize),
                     0);

    // --------------------------------------------------------
    // 2) Normalize: src * mean(background) / (background + eps)
    // --------------------------------------------------------
    cv::Mat src32f, bg32f;
    src.convertTo(src32f, CV_32F);
    background.convertTo(bg32f, CV_32F);

    cv::Scalar bgMean = cv::mean(bg32f);

    cv::Mat normalized32f =
        src32f.mul(bgMean[0]) / (bg32f + epsilon);

    // --------------------------------------------------------
    // 3) Normalize back to [0..255]
    // --------------------------------------------------------
    cv::Mat dst;
    cv::normalize(normalized32f,
                  dst,
                  0,
                  255,
                  cv::NORM_MINMAX,
                  CV_8UC1);

    return dst;
}

} // namespace Filters
} // namespace Preprocess
} // namespace Ocr
