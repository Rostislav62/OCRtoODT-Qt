// ============================================================
//  OCRtoODT — Poppler Service
//  File: popplerservice.cpp
// ============================================================

#include "PopplerService.h"

#include "core/LogRouter.h"

#include <poppler/qt6/poppler-qt6.h>
#include <memory>
#include <QElapsedTimer>

namespace Core {

// ------------------------------------------------------------
// Auto DPI for thumbnails (speed priority)
// ------------------------------------------------------------
double PopplerService::autoDpiForThumbnail(double wPts, double hPts)
{
    const double maxPx = 320.0;  // typical thumbnail resolution
    double dpi = maxPx / (qMax(wPts, hPts) / 72.0);

    return qBound(80.0, dpi, 140.0);
}

// ------------------------------------------------------------
// Auto DPI for preview (quality priority)
// ------------------------------------------------------------
double PopplerService::autoDpiForPreview(double wPts, double hPts)
{
    const double targetWidthPx = 1600.0;
    double dpi = targetWidthPx / (qMax(wPts, hPts) / 72.0);

    return qBound(150.0, dpi, 260.0);
}

// ------------------------------------------------------------
// DPI resolver (unifies logic)
// ------------------------------------------------------------
double PopplerService::resolveDpi(double dpiRequested,
                                  double wPts,
                                  double hPts)
{
    if (dpiRequested == 0)
        return autoDpiForPreview(wPts, hPts);

    if (dpiRequested < 0)
        return autoDpiForThumbnail(wPts, hPts);

    return dpiRequested;
}

// ------------------------------------------------------------
// Render PDF page (worker-safe, stateless)
// ------------------------------------------------------------
QImage PopplerService::renderPage(const QString &pdfPath,
                                  int pageIndex,
                                  double dpiRequested)
{
    QElapsedTimer timer;
    timer.start();

    // Load PDF
    std::unique_ptr<Poppler::Document> doc(
        Poppler::Document::load(pdfPath)
        );

    if (!doc)
    {
        LogRouter::instance().error(
            QString("[PopplerService] ERROR: cannot load PDF: %1").arg(pdfPath));
        return QImage();
    }

    const int total = doc->numPages();
    if (pageIndex < 0 || pageIndex >= total)
    {
        LogRouter::instance().error(
            QString("[PopplerService] ERROR: invalid page index %1/%2 (%3)")
                .arg(pageIndex).arg(total).arg(pdfPath));
        return QImage();
    }

    // Load page object
    std::unique_ptr<Poppler::Page> page(doc->page(pageIndex));
    if (!page)
    {
        LogRouter::instance().error(
            QString("[PopplerService] ERROR: cannot load page %1 (%2)")
                .arg(pageIndex).arg(pdfPath));
        return QImage();
    }

    // Page size (points)
    const QSizeF pts = page->pageSizeF();
    const double wPts = pts.width();
    const double hPts = pts.height();

    const double dpi = resolveDpi(dpiRequested, wPts, hPts);

    LogRouter::instance().info(
        QString("[PopplerService] Render start: %1 page %2  DPI=%3  sizePts=%.1fx%.1f")
            .arg(pdfPath)
            .arg(pageIndex + 1)
            .arg(dpi)
            .arg(wPts)
            .arg(hPts));

    QImage img = page->renderToImage(dpi, dpi);

    const qint64 ms = timer.elapsed();

    if (img.isNull())
    {
        LogRouter::instance().error(
            QString("[PopplerService] ERROR: rendering failed (%1 page %2, DPI=%3)")
                .arg(pdfPath)
                .arg(pageIndex + 1)
                .arg(dpi));
    }
    else
    {
        LogRouter::instance().info(
            QString("[PopplerService] Rendered %1 page %2 in %3 ms (%4x%5 px)")
                .arg(pdfPath)
                .arg(pageIndex + 1)
                .arg(ms)
                .arg(img.width())
                .arg(img.height()));
    }

    return img;
}

} // namespace Core
