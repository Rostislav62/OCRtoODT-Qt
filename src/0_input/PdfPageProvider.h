// ============================================================
//  OCRtoODT — PDF Page Provider
//  File: pdfpageprovider.h
//
//  Responsibility:
//      Asynchronous rendering of PDF pages using Poppler-Qt6.
//      Designed for UI preview / thumbnails only.
//
//      Key properties:
//          * NO heavy work in GUI thread
//          * Each request loads its own Poppler::Document
//            inside a QtConcurrent task (safe pattern)
//          * No shared Poppler::Document between threads
//            → avoids thread-safety problems.
//          * Results (QImage / QPixmap) are delivered via
//            Qt signals, queued back into the main thread.
//
//      Public API:
//          - requestThumbnail(pdfPath, pageIndex, maxSize, dpiHint)
//          - requestPageImage(pdfPath, pageIndex, dpi)
//
//      Signals:
//          - thumbnailReady(pdfPath, pageIndex, QPixmap)
//          - pageReady(pdfPath, pageIndex, QImage)
//
//      NOTE:
//          At the current stage of the project the main
//          preprocessing pipeline uses PNG pages generated
//          by DocumentController. PdfPageProvider is kept
//          as a reusable, self-contained component for
//          future PDF preview scenarios.
// ============================================================

#ifndef PDFPAGEPROVIDER_H
#define PDFPAGEPROVIDER_H

#include <QObject>
#include <QSize>

class QPixmap;
class QImage;

namespace Input {

class PdfPageProvider : public QObject
{
    Q_OBJECT

public:
    explicit PdfPageProvider(QObject *parent = nullptr);
    ~PdfPageProvider() override = default;

    // --------------------------------------------------------
    // Request asynchronous thumbnail
    //
    // pdfPath    - absolute path to PDF file
    // pageIndex  - 0-based page index
    // maxSize    - maximum thumbnail size (width/height)
    // dpiHint    - optional DPI for rendering (0.0 → auto 96 DPI)
    // --------------------------------------------------------
    void requestThumbnail(const QString &pdfPath,
                          int            pageIndex,
                          const QSize   &maxSize,
                          double         dpiHint = 0.0);

    // --------------------------------------------------------
    // Request asynchronous full-page image render
    //
    // pdfPath    - absolute path to PDF file
    // pageIndex  - 0-based page index
    // dpi        - render DPI (0.0 → reasonable default 150)
    // --------------------------------------------------------
    void requestPageImage(const QString &pdfPath,
                          int            pageIndex,
                          double         dpi = 0.0);

signals:
    // Thumbnail ready (scaled QPixmap)
    void thumbnailReady(const QString &pdfPath,
                        int            pageIndex,
                        const QPixmap &pix);

    // Full page image ready (raw QImage)
    void pageReady(const QString &pdfPath,
                   int            pageIndex,
                   const QImage  &image);
};

} // namespace Input

#endif // PDFPAGEPROVIDER_H
