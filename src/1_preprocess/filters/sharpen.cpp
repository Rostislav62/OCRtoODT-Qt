// ============================================================
//  OCRtoODT — Preprocess Filters: Sharpen (Unsharp Mask)
//  File: 1_preprocess/filters/sharpen.cpp
//
//  Implementation details:
//      - Unsharp mask = original + strength * (original - blurred)
//      - Uses Gaussian blur internally
//      - Operates ONLY on grayscale images
//
//  WARNING:
//      This filter can easily DAMAGE OCR results.
// ============================================================

#include "1_preprocess/filters/sharpen.h"

#include <opencv2/imgproc.hpp>

namespace Ocr {
namespace Preprocess {
namespace Filters {

cv::Mat unsharpMask(const cv::Mat &src,
                    double strength,
                    int gaussianKSize,
                    double gaussianSigma)
{
    // Defensive checks
    if (src.empty())
        return cv::Mat();

    if (src.channels() != 1)
        return src.clone();

    if (strength <= 0.0)
        return src.clone();

    // Sanitize parameters
    if (gaussianKSize < 3)
        gaussianKSize = 3;
    if ((gaussianKSize % 2) == 0)
        gaussianKSize += 1;

    if (gaussianSigma < 0.0)
        gaussianSigma = 0.0;

    // --------------------------------------------------------
    // 1) Blur image
    // --------------------------------------------------------
    cv::Mat blurred;
    cv::GaussianBlur(src,
                     blurred,
                     cv::Size(gaussianKSize, gaussianKSize),
                     gaussianSigma);

    // --------------------------------------------------------
    // 2) Unsharp mask: src + strength * (src - blurred)
    // --------------------------------------------------------
    cv::Mat src32f, blurred32f;
    src.convertTo(src32f, CV_32F);
    blurred.convertTo(blurred32f, CV_32F);

    cv::Mat mask = src32f - blurred32f;
    cv::Mat sharpened32f = src32f + strength * mask;

    // --------------------------------------------------------
    // 3) Convert back to 8-bit with saturation
    // --------------------------------------------------------
    cv::Mat dst;
    sharpened32f.convertTo(dst, CV_8UC1);

    return dst;
}

} // namespace Filters
} // namespace Preprocess
} // namespace Ocr
