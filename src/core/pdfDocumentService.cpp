// ============================================================
//  OCRtoODT — PDF Document Service
//  File: core/pdfdocumentservice.cpp
//
//  Responsibility:
//      Centralized access layer for PDF documents via Poppler.
//      - Caches loaded Poppler::Document instances
//      - Provides shared access to documents and pages
//      - Ensures thread-safe reuse across the application
//
//  Logging policy (STEP 6.1.5.4.3.1):
//      ❗ All logging MUST go through LogRouter
//      ❗ No direct qDebug / qWarning / qCritical calls allowed
// ============================================================

#include "core/pdfDocumentService.h"

#include "core/LogRouter.h"

#include <poppler/qt6/poppler-qt6.h>
#include <QtMath>

// ------------------------------------------------------------
// Singleton
// ------------------------------------------------------------
PdfDocumentService &PdfDocumentService::instance()
{
    static PdfDocumentService inst;
    return inst;
}

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
PdfDocumentService::PdfDocumentService(QObject *parent)
    : QObject(parent)
{
}

// ------------------------------------------------------------
// Load or retrieve Poppler::Document
//
// Behavior:
//   - Thread-safe access via mutex
//   - Cached documents are reused
//   - Poppler::Document::load() returns std::unique_ptr
//   - Internally converted to std::shared_ptr for caching
// ------------------------------------------------------------
std::shared_ptr<Poppler::Document>
PdfDocumentService::getDocument(const QString &pdfPath)
{
    QMutexLocker locker(&m_mutex);

    auto it = m_documents.find(pdfPath);
    if (it != m_documents.end() && it.value())
        return it.value();

    // Poppler API: load() returns std::unique_ptr<Document>
    std::unique_ptr<Poppler::Document> uniqueDoc =
        Poppler::Document::load(pdfPath);

    if (!uniqueDoc)
    {
        // STEP 6.1.5 — unified logging via LogRouter
        LogRouter::instance().warning(
            QString("[PdfDocumentService] Failed to open PDF: %1")
                .arg(pdfPath)
            );
        return {};
    }

    // Convert unique_ptr → shared_ptr
    // Ownership is transferred to shared_ptr with explicit deleter
    Poppler::Document *raw = uniqueDoc.release();

    std::shared_ptr<Poppler::Document> doc(
        raw,
        [](Poppler::Document *p)
        {
            delete p;
        });

    m_documents.insert(pdfPath, doc);
    return doc;
}

// ------------------------------------------------------------
// Get page from document
//
// Poppler specifics:
//   - Document::numPages() returns page count
//   - Document::page(int) returns std::unique_ptr<Page>
//   - Pages are owned by Poppler::Document and must NOT be deleted
// ------------------------------------------------------------
std::shared_ptr<Poppler::Page>
PdfDocumentService::getPage(const QString &pdfPath, int pageIndex)
{
    auto doc = getDocument(pdfPath);
    if (!doc)
        return {};

    int total = doc->numPages();
    if (pageIndex < 0 || pageIndex >= total)
        return {};

    // Poppler API: page() → std::unique_ptr<Page>
    std::unique_ptr<Poppler::Page> uniquePage =
        doc->page(pageIndex);

    if (!uniquePage)
        return {};

    Poppler::Page *raw = uniquePage.release();

    // Wrap page into shared_ptr WITHOUT deleting it
    // Poppler::Document manages page lifetime
    std::shared_ptr<Poppler::Page> page(
        raw,
        [](Poppler::Page *)
        {
            // intentionally empty — do NOT delete
        });

    return page;
}

// ------------------------------------------------------------
// Get page size in millimeters
//
// Notes:
//   - Poppler returns size in points (1/72 inch)
//   - Conversion: points → inches → millimeters
// ------------------------------------------------------------
QSizeF PdfDocumentService::pageSizeMm(const QString &pdfPath, int pageIndex)
{
    auto page = getPage(pdfPath, pageIndex);
    if (!page)
        return {};

    QSizeF pts = page->pageSizeF(); // points (1/72 inch)

    const double inchToMm = 25.4;
    return QSizeF(
        pts.width()  * inchToMm / 72.0,
        pts.height() * inchToMm / 72.0
        );
}
