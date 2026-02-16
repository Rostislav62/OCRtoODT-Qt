// ============================================================
//  OCRtoODT — Preprocess Filters: CLAHE
//  File: 1_preprocess/filters/clahe.cpp
//
//  Implementation details:
//      - Uses OpenCV CLAHE implementation
//      - Operates ONLY on grayscale images
//      - Designed to be called conditionally by EnhanceProcessor
//
//  IMPORTANT:
//      CLAHE modifies local contrast, not global brightness.
// ============================================================

#include "1_preprocess/filters/clahe.h"

#include <opencv2/imgproc.hpp>

namespace Ocr {
namespace Preprocess {
namespace Filters {

cv::Mat applyClahe(const cv::Mat &src,
                   double clipLimit,
                   int tileGridSize)
{
    // Defensive checks
    if (src.empty())
        return cv::Mat();

    if (src.channels() != 1)
        return src.clone();

    // Sanitize parameters
    if (clipLimit < 1.0)
        clipLimit = 1.0;

    if (tileGridSize < 2)
        tileGridSize = 2;

    // Create CLAHE object
    cv::Ptr<cv::CLAHE> clahe =
        cv::createCLAHE(clipLimit,
                        cv::Size(tileGridSize, tileGridSize));

    cv::Mat dst;
    clahe->apply(src, dst);

    return dst;
}

} // namespace Filters
} // namespace Preprocess
} // namespace Ocr
