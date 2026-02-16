// ============================================================
//  OCRtoODT — ODT Exporter (STEP 5.5)
//  File: src/5_export/odt_export/OdtExporter.h
//
//  Responsibility:
//      Export Step5::DocumentModel into ODT using external ZIP.
//
//  Notes:
//      - No Qt private API
//      - Uses system `zip`
// ============================================================

#pragma once

#include <QString>
#include "5_document/DocumentModel.h"

namespace Export {

class OdtExporter
{
public:
    static bool writeOdtFile(const Step5::DocumentModel &document,
                             const OdtLayoutModel       &layout,
                             const QString              &outputPath);

};

} // namespace Export
