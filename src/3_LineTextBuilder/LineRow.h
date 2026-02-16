// ============================================================
//  OCRtoODT — STEP 3: LineRow (Editable OCR Line)
//  File: src/3_LineTextBuilder/LineRow.h
//
//  Responsibility:
//      Immutable geometry + editable text representation of one OCR line,
//      produced by STEP 3 from RAW TSV (RAM).
//
//  Design rules:
//      - text is editable by user
//      - bbox NEVER changes after build
//      - empty lines are allowed (text="") and participate in lineOrder
// ============================================================

#ifndef TSV_LINEROW_H
#define TSV_LINEROW_H

#include <QString>
#include <QRect>

namespace Tsv {

struct LineRow
{
    // --------------------------------------------------------
    // Identity
    // --------------------------------------------------------
    int     pageIndex  = -1;   // Core::VirtualPage.globalIndex
    int     lineOrder  = -1;   // absolute order on page (including empty lines)

    // --------------------------------------------------------
    // Editable text
    // --------------------------------------------------------
    QString text;             // user-editable line text

    // --------------------------------------------------------
    // Geometry (image coordinates)
    // --------------------------------------------------------
    QRect   bbox;             // line bbox in image pixels

    // --------------------------------------------------------
    // TSV lineage (source identifiers)
    // --------------------------------------------------------
    int blockNum = -1;
    int parNum   = -1;
    int lineNum  = -1;

    // --------------------------------------------------------
    // Quality metrics (computed from level 5 words)
    // --------------------------------------------------------
    double  avgConf   = 0.0;
    int     wordCount = 0;

    // --------------------------------------------------------
    // Helpers
    // --------------------------------------------------------
    bool isEmptyLine() const { return text.trimmed().isEmpty(); }
};

} // namespace Tsv

#endif // TSV_LINEROW_H
