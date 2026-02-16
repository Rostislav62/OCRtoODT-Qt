// ============================================================
//  OCRtoODT — DOCX Exporter (STEP 5.6)
//  File: src/5_export/docx_export/DocxExporter.h
//
//  Responsibility:
//      Export Step5::DocumentModel to minimal valid DOCX file.
//      MVP: paragraphs + page breaks, no styles.
// ============================================================

#pragma once

#include <QString>

#include "5_document/DocumentModel.h"
#include "core/layout/OdtLayoutModel.h"


namespace Export {

class DocxExporter
{
public:
    // ============================================================
    //  Public API
    // ============================================================

    static bool writeDocxFile(
        const Step5::DocumentModel &document,
        const OdtLayoutModel       &layout,
        const QString              &outputPath);
};

} // namespace Export
