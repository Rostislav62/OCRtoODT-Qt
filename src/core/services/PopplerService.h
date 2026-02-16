// ============================================================
//  OCRtoODT — Poppler Service
//  File: popplerservice.h
//
//  Responsibility:
//      High-level stateless PDF rendering helper using Poppler-Qt6.
//
//      Features:
//          • Auto DPI selection (preview / thumbnail modes)
//          • Stable worker-safe API (no shared Poppler::Document)
//          • Unified logging via LogRouter
//          • Graceful error handling
//
//      NOTE:
//          All API is static; no caching here. Caching is planned
//          at pipeline level (OcrPdfCache).
// ============================================================

#ifndef POPPLERSERVICE_H
#define POPPLERSERVICE_H

#include <QString>
#include <QImage>
#include <QSizeF>

namespace Core {

class PopplerService
{
public:
    // Render PDF page.
    //
    // dpiRequested = 0     → auto DPI for preview
    // dpiRequested < 0     → auto DPI for thumbnail
    // dpiRequested > 0     → use exact DPI
    //
    // Returns null QImage on failure.
    static QImage renderPage(const QString &pdfPath,
                             int pageIndex,
                             double dpiRequested);

private:
    // ----- Auto DPI helpers -----
    static double autoDpiForThumbnail(double wPts, double hPts);
    static double autoDpiForPreview(double wPts, double hPts);

    // Internal helper: calculate auto DPI based on mode
    static double resolveDpi(double dpiRequested,
                             double wPts,
                             double hPts);
};

} // namespace Core

#endif // POPPLERSERVICE_H
