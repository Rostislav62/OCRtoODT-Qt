// ============================================================
//  OCRtoODT — Preprocess Filters: Sauvola Binarization
//  File: 1_preprocess/filters/sauvola.cpp
//
//  Implementation details:
//      - Implements classic Sauvola thresholding
//      - Uses integral images for mean and variance
//      - Produces a strictly binary output
//
//  WARNING:
//      Output is binary; grayscale information is lost forever.
// ============================================================

#include "1_preprocess/filters/sauvola.h"

#include <opencv2/imgproc.hpp>
#include <cmath>

namespace Ocr {
namespace Preprocess {
namespace Filters {

cv::Mat sauvolaBinarize(const cv::Mat &src,
                        int windowSize,
                        double k,
                        double R)
{
    // Defensive checks
    if (src.empty())
        return cv::Mat();

    if (src.channels() != 1)
        return src.clone();

    // Sanitize parameters
    if (windowSize < 3)
        windowSize = 3;
    if ((windowSize % 2) == 0)
        windowSize += 1;

    if (k < 0.0)
        k = 0.0;

    if (R <= 0.0)
        R = 128.0;

    const int half = windowSize / 2;

    // --------------------------------------------------------
    // Prepare integral images
    // --------------------------------------------------------
    cv::Mat src32f;
    src.convertTo(src32f, CV_32F);

    cv::Mat integral, integralSq;
    cv::integral(src32f, integral, integralSq, CV_64F);

    cv::Mat dst(src.size(), CV_8UC1);

    const int rows = src.rows;
    const int cols = src.cols;

    // --------------------------------------------------------
    // Sauvola thresholding
    // --------------------------------------------------------
    for (int y = 0; y < rows; ++y) {
        int y0 = std::max(0, y - half);
        int y1 = std::min(rows - 1, y + half);

        for (int x = 0; x < cols; ++x) {
            int x0 = std::max(0, x - half);
            int x1 = std::min(cols - 1, x + half);

            int area = (x1 - x0 + 1) * (y1 - y0 + 1);

            double sum =
                integral.at<double>(y1 + 1, x1 + 1)
                - integral.at<double>(y0,     x1 + 1)
                - integral.at<double>(y1 + 1, x0)
                + integral.at<double>(y0,     x0);

            double sumSq =
                integralSq.at<double>(y1 + 1, x1 + 1)
                - integralSq.at<double>(y0,     x1 + 1)
                - integralSq.at<double>(y1 + 1, x0)
                + integralSq.at<double>(y0,     x0);

            double mean = sum / area;
            double variance = (sumSq / area) - (mean * mean);
            double stddev = std::sqrt(std::max(variance, 0.0));

            double threshold =
                mean * (1.0 + k * ((stddev / R) - 1.0));

            dst.at<uchar>(y, x) =
                (src.at<uchar>(y, x) > threshold) ? 255 : 0;
        }
    }

    return dst;
}

} // namespace Filters
} // namespace Preprocess
} // namespace Ocr
