// ============================================================
//  OCRtoODT — STEP 5.2: DocumentBuilder (LineTable → DocumentModel)
//  File: src/5_document/DocumentBuilder.h
//
//  Responsibility:
//      Convert VirtualPage collection into pure in-memory
//      DocumentModel representation.
//
//  Design rules:
//      - No UI
//      - No file I/O
//      - Deterministic structure
//      - PageBreak blocks are NOT stored anymore
//        (exporters derive page breaks from pageIndex transitions)
// ============================================================

#pragma once

#include <QVector>

#include "5_document/DocumentModel.h"
#include "core/VirtualPage.h"

namespace Step5 {

class DocumentBuilder
{
public:

    // ============================================================
    // Build final DocumentModel
    // ============================================================
    static DocumentModel build(const QVector<Core::VirtualPage> &pages,
                               const DocumentBuildOptions       &opt);

private:

    // ------------------------------------------------------------
    // Sorting helper
    // ------------------------------------------------------------
    static QVector<Core::VirtualPage> sortedByGlobalIndex(
        const QVector<Core::VirtualPage> &pages);

    // ------------------------------------------------------------
    // Append page content as blocks
    // (pageIndex is now explicitly passed)
    // ------------------------------------------------------------
    static void appendPageAsBlocks(const Core::VirtualPage    &vp,
                                   const DocumentBuildOptions &opt,
                                   DocumentModel              *out,
                                   int                         pageIndex);

    // ------------------------------------------------------------
    // MVP: each line → paragraph
    // ------------------------------------------------------------
    static void appendPage_MvpLinePerParagraph(
        const Core::VirtualPage    &vp,
        const DocumentBuildOptions &opt,
        DocumentModel              *out,
        int                         pageIndex);

    // ------------------------------------------------------------
    // STEP 3 markers segmentation
    // ------------------------------------------------------------
    static void appendPage_FromStep3Markers(
        const Core::VirtualPage    &vp,
        const DocumentBuildOptions &opt,
        DocumentModel              *out,
        int                         pageIndex);

    // ------------------------------------------------------------
    // Push paragraph into model
    // (stores pageIndex inside block)
    // ------------------------------------------------------------
    static void pushParagraph(DocumentModel *out,
                              const QString &text,
                              int            pageIndex);
};

} // namespace Step5
