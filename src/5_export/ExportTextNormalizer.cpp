// ============================================================
//  OCRtoODT — ExportTextNormalizer
//  File: src/5_export/ExportTextNormalizer.cpp
//
//  Responsibility:
//      Normalize DocumentModel before export:
//          • apply maxEmptyLines rule
//          • keep pure paragraph structure
//
//  NOTE:
//      Page breaks are NOT stored in DocumentModel.
//      They are derived in exporters via pageIndex transitions.
// ============================================================

#include "5_export/ExportTextNormalizer.h"

namespace Export {

Step5::DocumentModel
ExportTextNormalizer::normalize(const Step5::DocumentModel &input,
                                int                         maxEmptyLines)
{
    Step5::DocumentModel out;
    out.options = input.options;

    int emptyRun = 0;

    for (const auto &block : input.blocks)
    {
        const bool isEmpty = block.text.trimmed().isEmpty();

        if (isEmpty)
        {
            ++emptyRun;

            if (emptyRun > maxEmptyLines)
                continue;
        }
        else
        {
            emptyRun = 0;
        }

        out.blocks.push_back(block);
    }

    return out;
}

} // namespace Export
