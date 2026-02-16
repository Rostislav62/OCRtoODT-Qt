// ============================================================
//  OCRtoODT — Preprocess: Image Analyzer
//  File: src/1_preprocess/ImageAnalyzer.cpp
// ============================================================

#include "1_preprocess/ImageAnalyzer.h"

#include <opencv2/imgproc.hpp>
#include <algorithm>

#include "core/ConfigManager.h"
#include "core/LogRouter.h"

namespace Ocr {
namespace Preprocess {

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------
static cv::Mat qimageToGrayMat(const QImage &img)
{
    if (img.isNull())
        return {};

    QImage rgb = img;
    if (rgb.format() != QImage::Format_RGB888)
        rgb = rgb.convertToFormat(QImage::Format_RGB888);

    cv::Mat cvRgb(
        rgb.height(),
        rgb.width(),
        CV_8UC3,
        const_cast<uchar*>(rgb.bits()),
        rgb.bytesPerLine());

    cv::Mat gray;
    cv::cvtColor(cvRgb, gray, cv::COLOR_RGB2GRAY);
    return gray.clone();
}

static bool looksBinaryFast(const cv::Mat &gray)
{
    if (gray.empty())
        return false;

    cv::Mat hist;
    int histSize = 256;
    float range[] = {0, 256};
    const float *ranges[] = {range};
    int channels[] = {0};

    cv::calcHist(&gray, 1, channels, cv::Mat(),
                 hist, 1, &histSize, ranges);

    const double total = gray.total();
    const double nearBlack =
        hist.at<float>(0) + hist.at<float>(1) + hist.at<float>(2);
    const double nearWhite =
        hist.at<float>(253) + hist.at<float>(254) + hist.at<float>(255);

    return ((nearBlack + nearWhite) / std::max(total, 1.0)) > 0.85;
}

// ------------------------------------------------------------
// DPI POLICY (CENTRALIZED HERE)
// ------------------------------------------------------------
int ImageAnalyzer::deriveOcrDpi(int longSidePx)
{
    ConfigManager &cfg = ConfigManager::instance();

    const int dpiDefault =
        cfg.get("ocr.dpi_default", 300).toInt();

    const int dpiLowResThreshold =
        cfg.get("ocr.dpi_low_res_threshold", 1500).toInt();

    const int dpiLowResValue =
        cfg.get("ocr.dpi_low_res_value", 96).toInt();

    if (longSidePx > 0 && longSidePx < dpiLowResThreshold)
        return dpiLowResValue;

    return dpiDefault;
}

// ------------------------------------------------------------
// Analyze grayscale image
// ------------------------------------------------------------
ImageDiagnostics ImageAnalyzer::analyzeGray(const cv::Mat &gray)
{
    ImageDiagnostics d;

    if (gray.empty() || gray.type() != CV_8UC1)
        return d;

    d.widthPx  = gray.cols;
    d.heightPx = gray.rows;
    d.longSidePx = std::max(d.widthPx, d.heightPx);
    d.looksBinary = looksBinaryFast(gray);

    cv::Mat lap;
    cv::Laplacian(gray, lap, CV_64F);
    cv::Scalar mean, stddev;
    cv::meanStdDev(lap, mean, stddev);
    d.blurScore = stddev[0] * stddev[0];

    cv::Mat blur;
    cv::GaussianBlur(gray, blur, cv::Size(3, 3), 0.0);
    cv::Mat hp;
    cv::absdiff(gray, blur, hp);
    cv::meanStdDev(hp, mean, stddev);
    d.noiseScore = stddev[0];

    cv::Mat bg;
    cv::GaussianBlur(gray, bg, cv::Size(51, 51), 0.0);
    cv::meanStdDev(bg, mean, stddev);
    d.backgroundVariance = stddev[0];

    d.suggestedOcrDpi = deriveOcrDpi(d.longSidePx);

    LogRouter::instance().debug(
        QString("[ImageAnalyzer] size=%1x%2 long=%3 dpi=%4")
            .arg(d.widthPx)
            .arg(d.heightPx)
            .arg(d.longSidePx)
            .arg(d.suggestedOcrDpi));

    return d;
}

// ------------------------------------------------------------
// Analyze QImage
// ------------------------------------------------------------
ImageDiagnostics ImageAnalyzer::analyzeQImage(const QImage &img)
{
    return analyzeGray(qimageToGrayMat(img));
}

} // namespace Preprocess
} // namespace Ocr
