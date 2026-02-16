// ============================================================
//  OCRtoODT — Export Text Normalizer
//  File: src/5_export/ExportTextNormalizer.h
//
//  Responsibility:
//      Provide deterministic structural normalization for
//      Step5::DocumentModel before export.
//
//  What it does:
//      - Limits number of consecutive empty paragraphs
//      - Preserves PageBreak blocks
//      - Does NOT modify original model
//
//  Used by:
//      - OdtExporter
//      - DocxExporter
//      - TxtExporter
//
//  Notes:
//      - Pure utility (stateless)
//      - No dependency on UI
//      - No dependency on ConfigManager
// ============================================================

#pragma once

#include "5_document/DocumentModel.h"

namespace Export {

class ExportTextNormalizer
{
public:
    // --------------------------------------------------------
    // Normalize DocumentModel according to layout rules
    //
    // Parameters:
    //      input           — original document model
    //      maxEmptyLines   — maximum allowed consecutive
    //                        empty paragraphs (0–3)
    //
    // Returns:
    //      New normalized DocumentModel
    // --------------------------------------------------------
    static Step5::DocumentModel normalize(
        const Step5::DocumentModel &input,
        int maxEmptyLines
        );
};

} // namespace Export
