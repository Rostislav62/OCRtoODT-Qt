// ============================================================
//  OCRtoODT — Preprocess Filters: Gaussian Blur
//  File: 1_preprocess/filters/gaussian.cpp
//
//  Implementation details:
//      - Thin wrapper around cv::GaussianBlur
//      - Ensures valid kernel size
//      - Operates ONLY on grayscale images
//
//  IMPORTANT:
//      This filter must never upconvert or binarize the image.
// ============================================================

#include "1_preprocess/filters/gaussian.h"

#include <opencv2/imgproc.hpp>

namespace Ocr {
namespace Preprocess {
namespace Filters {

cv::Mat gaussianBlur(const cv::Mat &src,
                     int kernelSize,
                     double sigma)
{
    // Defensive checks
    if (src.empty())
        return cv::Mat();

    if (src.channels() != 1)
        return src.clone();

    // Ensure kernel size is valid
    if (kernelSize < 3)
        kernelSize = 3;
    if ((kernelSize % 2) == 0)
        kernelSize += 1;

    if (sigma < 0.0)
        sigma = 0.0;

    cv::Mat dst;
    cv::GaussianBlur(src,
                     dst,
                     cv::Size(kernelSize, kernelSize),
                     sigma);

    return dst;
}

} // namespace Filters
} // namespace Preprocess
} // namespace Ocr
