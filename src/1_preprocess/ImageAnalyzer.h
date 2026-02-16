// ============================================================
//  OCRtoODT — Preprocess: Image Analyzer
//  File: src/1_preprocess/ImageAnalyzer.h
//
//  Responsibility:
//      Analyze input image and compute objective diagnostics
//      used by preprocessing and OCR policies.
//
//      This class is the SINGLE place where:
//          • image resolution is measured
//          • OCR DPI is derived
//
//      IMPORTANT:
//          • Does NOT read global execution mode
//          • Does NOT modify PageJob
//          • Does NOT perform preprocessing
//
// ============================================================

#ifndef PREPROCESS_IMAGEANALYZER_H
#define PREPROCESS_IMAGEANALYZER_H

#include <QImage>
#include <opencv2/core.hpp>

namespace Ocr {
namespace Preprocess {

struct ImageDiagnostics
{
    int     widthPx  = 0;
    int     heightPx = 0;
    int     longSidePx = 0;

    double  blurScore = 0.0;
    double  noiseScore = 0.0;
    double  backgroundVariance = 0.0;
    bool    looksBinary = false;

    int     suggestedOcrDpi = 300;   // FINAL RESULT
};

class ImageAnalyzer
{
public:
    // --------------------------------------------------------
    // Analyze already-loaded grayscale image
    // --------------------------------------------------------
    static ImageDiagnostics analyzeGray(const cv::Mat &gray);

    // --------------------------------------------------------
    // Analyze QImage (used for source images)
    // --------------------------------------------------------
    static ImageDiagnostics analyzeQImage(const QImage &img);

private:
    static int deriveOcrDpi(int longSidePx);
};

} // namespace Preprocess
} // namespace Ocr

#endif // PREPROCESS_IMAGEANALYZER_H
