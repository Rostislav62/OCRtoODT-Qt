// ============================================================
//  OCRtoODT — PDF Document Service
//  File: pdfdocumentservice.h
//
//  Responsibility:
//      Thin singleton wrapper around Poppler-Qt6 that:
//          - loads PDF documents
//          - caches Poppler::Document per file path
//          - gives access to Poppler::Page
//          - provides page size in millimetres
//
//      This allows PdfPageProvider to avoid reopening the same
//      PDF for every page render.
// ============================================================

#ifndef PDFDOCUMENTSERVICE_H
#define PDFDOCUMENTSERVICE_H

#include <QObject>
#include <QHash>
#include <QMutex>
#include <QSizeF>

#include <memory>

// Forward declaration (from Poppler-Qt6)
namespace Poppler {
class Document;
class Page;
}

class PdfDocumentService : public QObject
{
    Q_OBJECT

public:
    // --------------------------------------------------------
    // Singleton access
    // --------------------------------------------------------
    static PdfDocumentService &instance();

    // --------------------------------------------------------
    // Get (or load) a Poppler::Document for the given path.
    // Returns nullptr on error.
    // --------------------------------------------------------
    std::shared_ptr<Poppler::Document> getDocument(const QString &pdfPath);

    // --------------------------------------------------------
    // Get page object for (pdfPath, pageIndex).
    // Returns nullptr on error or invalid index.
    // --------------------------------------------------------
    std::shared_ptr<Poppler::Page> getPage(const QString &pdfPath,
                                           int pageIndex);

    // --------------------------------------------------------
    // Get logical page size in millimetres for (pdfPath, pageIndex).
    // Returns invalid QSizeF on error.
    // --------------------------------------------------------
    QSizeF pageSizeMm(const QString &pdfPath, int pageIndex);

private:
    explicit PdfDocumentService(QObject *parent = nullptr);
    Q_DISABLE_COPY(PdfDocumentService)

private:
    QMutex m_mutex;  // protects m_documents
    QHash<QString, std::shared_ptr<Poppler::Document>> m_documents;
};

#endif // PDFDOCUMENTSERVICE_H
