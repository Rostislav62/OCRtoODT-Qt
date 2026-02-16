// ============================================================
//  OCRtoODT — STEP 4: Line Hit Testing (Preview → Text mapping)
//  File: src/4_edit_lines/LineHitTest.h
//
//  Responsibility:
//      Map an image-space point (pixel coords) to a LineRow index
//      using bbox containment (simple linear scan).
// ============================================================

#pragma once

#include <QPoint>

namespace Tsv {
struct LineTable;
}

class LineHitTest
{
public:
    // Returns model row index, or -1 if none.
    static int hitTest(const Tsv::LineTable *table, const QPoint &imagePos);
};
