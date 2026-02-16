// ============================================================
//  OCRtoODT — PDF Page Provider
//  File: PdfPageProvider.cpp
//
//  Responsibility:
//      Asynchronous Poppler-based rendering of PDF pages
//      for thumbnails and full-page previews.
//
//      Implementation notes:
//          * Each request runs in QtConcurrent::run().
//          * Inside the task we load Poppler::Document,
//            extract a single page and render it.
//          * There is NO shared Poppler::Document between
//            tasks → safe for multi-threaded rendering.
//          * Signals are emitted from worker threads;
//            Qt queues them back to the receiver thread.
//
//      Usage pattern (example):
//          PdfPageProvider *provider = new PdfPageProvider(this);
//          connect(provider, &PdfPageProvider::thumbnailReady, ...);
//          provider->requestThumbnail(pdfPath, 0, QSize(160,160));
//
// ============================================================

#include "PdfPageProvider.h"

// Core logging
#include "core/LogRouter.h"

// Qt
#include <QImage>
#include <QPixmap>
#include <QtConcurrent>

// Poppler
#include <poppler/qt6/poppler-qt6.h>
#include <memory>

using Input::PdfPageProvider;

static inline void logInfo(const QString &m)    { LogRouter::instance().info(m); }
static inline void logError(const QString &m)   { LogRouter::instance().error(m); }
static inline void logWarning(const QString &m) { LogRouter::instance().warning(m); }

// ============================================================
// Constructor
// ============================================================
PdfPageProvider::PdfPageProvider(QObject *parent)
    : QObject(parent)
{
    logInfo("[PdfPageProvider] Initialized.");
}

// ============================================================
// requestThumbnail
// ============================================================
void PdfPageProvider::requestThumbnail(const QString &pdfPath,
                                       int            pageIndex,
                                       const QSize   &maxSize,
                                       double         dpiHint)
{
    // Run in global thread pool, no blocking of GUI
    QtConcurrent::run([=]()
                      {
                          if (pdfPath.isEmpty() || !maxSize.isValid())
                          {
                              logWarning("[PdfPageProvider] requestThumbnail: invalid parameters.");
                              return;
                          }

                          std::unique_ptr<Poppler::Document> pdf(Poppler::Document::load(pdfPath));
                          if (!pdf)
                          {
                              logError(QString("[PdfPageProvider] Failed to open PDF: %1").arg(pdfPath));
                              return;
                          }

                          if (pageIndex < 0 || pageIndex >= pdf->numPages())
                          {
                              logWarning(QString("[PdfPageProvider] Page index out of range: %1").arg(pageIndex));
                              return;
                          }

                          std::unique_ptr<Poppler::Page> page(pdf->page(pageIndex));
                          if (!page)
                          {
                              logError(QString("[PdfPageProvider] Failed to load page %1 from %2")
                                           .arg(pageIndex).arg(pdfPath));
                              return;
                          }

                          // Choose DPI
                          double dpi = dpiHint <= 0.0 ? 96.0 : dpiHint;
                          QImage image = page->renderToImage(dpi, dpi);
                          if (image.isNull())
                          {
                              logError(QString("[PdfPageProvider] Null render (thumbnail) for %1 page %2")
                                           .arg(pdfPath).arg(pageIndex));
                              return;
                          }

                          // Scale to thumbnail size
                          QImage scaled = image.scaled(maxSize,
                                                       Qt::KeepAspectRatio,
                                                       Qt::SmoothTransformation);

                          QPixmap pix = QPixmap::fromImage(scaled);

                          // Emit signal (Qt will deliver to receiver's thread)
                          emit thumbnailReady(pdfPath, pageIndex, pix);
                      });
}

// ============================================================
// requestPageImage
// ============================================================
void PdfPageProvider::requestPageImage(const QString &pdfPath,
                                       int            pageIndex,
                                       double         dpi)
{
    QtConcurrent::run([=]()
                      {
                          if (pdfPath.isEmpty())
                          {
                              logWarning("[PdfPageProvider] requestPageImage: empty path.");
                              return;
                          }

                          std::unique_ptr<Poppler::Document> pdf(Poppler::Document::load(pdfPath));
                          if (!pdf)
                          {
                              logError(QString("[PdfPageProvider] Failed to open PDF: %1").arg(pdfPath));
                              return;
                          }

                          if (pageIndex < 0 || pageIndex >= pdf->numPages())
                          {
                              logWarning(QString("[PdfPageProvider] Page index out of range: %1").arg(pageIndex));
                              return;
                          }

                          std::unique_ptr<Poppler::Page> page(pdf->page(pageIndex));
                          if (!page)
                          {
                              logError(QString("[PdfPageProvider] Failed to load page %1 from %2")
                                           .arg(pageIndex).arg(pdfPath));
                              return;
                          }

                          double effectiveDpi = dpi <= 0.0 ? 150.0 : dpi;

                          QImage image = page->renderToImage(effectiveDpi, effectiveDpi);
                          if (image.isNull())
                          {
                              logError(QString("[PdfPageProvider] Null render (page) for %1 page %2")
                                           .arg(pdfPath).arg(pageIndex));
                              return;
                          }

                          emit pageReady(pdfPath, pageIndex, image);
                      });
}
