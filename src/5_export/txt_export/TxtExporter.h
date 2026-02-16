// ============================================================
//  OCRtoODT — TXT Exporter
//  File: src/5_export/txt_export/TxtExporter.h
//
//  Responsibility:
//      Serialize Step5::DocumentModel into plain UTF-8 TXT.
//
//  Notes:
//      - Formatting is minimal and deterministic
//      - Paragraphs separated by blank line
// ============================================================

#pragma once

#include <QString>

#include "5_document/DocumentModel.h"
#include "core/layout/OdtLayoutModel.h"


namespace Export {

class TxtExporter
{
public:
    static bool writeTxtFile(
        const Step5::DocumentModel &document,
        const OdtLayoutModel       &layout,
        const QString              &outputPath);

};

} // namespace Export
