// ============================================================
//  OCRtoODT — STEP 3: LineTable (Per-Page Editable Lines)
//  File: src/3_LineTextBuilder/LineTable.h
//
//  Responsibility:
//      Container for STEP 3 output: ordered editable text lines
//      with stable bbox mapping for OCR Text Tab interaction.
// ============================================================

#ifndef TSV_LINETABLE_H
#define TSV_LINETABLE_H

#include <QVector>
#include <QPoint>

#include "3_LineTextBuilder/LineRow.h"

namespace Tsv {

struct LineTable
{
    QVector<LineRow> rows;

    // --------------------------------------------------------
    // Basic helpers
    // --------------------------------------------------------
    int size() const { return rows.size(); }
    bool isEmpty() const { return rows.isEmpty(); }

    const LineRow* rowAt(int i) const
    {
        if (i < 0 || i >= rows.size())
            return nullptr;
        return &rows[i];
    }

    LineRow* rowAt(int i)
    {
        if (i < 0 || i >= rows.size())
            return nullptr;
        return &rows[i];
    }

    // --------------------------------------------------------
    // Hit-test: find line index by image pixel position
    // --------------------------------------------------------
    int hitTest(const QPoint &imagePos) const
    {
        // Prefer first match in reading order (stable)
        for (int i = 0; i < rows.size(); ++i)
        {
            const QRect &r = rows[i].bbox;
            if (!r.isNull() && r.contains(imagePos))
                return i;
        }
        return -1;
    }
};

} // namespace Tsv

#endif // TSV_LINETABLE_H
