// ============================================================
//  OCRtoODT — STEP 4: Line Hit Testing (Preview → Text mapping)
//  File: src/4_edit_lines/LineHitTest.cpp
// ============================================================

#include "4_edit_lines/LineHitTest.h"
#include "3_LineTextBuilder/LineTable.h"

int LineHitTest::hitTest(const Tsv::LineTable *table, const QPoint &imagePos)
{
    if (!table)
        return -1;

    // Linear scan is sufficient for typical OCR line counts.
    // Synthetic empty lines have empty QRect() => never match.
    for (int i = 0; i < table->rows.size(); ++i)
    {
        const auto &row = table->rows[i];
        if (!row.bbox.isNull() && row.bbox.contains(imagePos))
            return i;
    }

    return -1;
}
