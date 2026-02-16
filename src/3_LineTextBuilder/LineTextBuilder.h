// ============================================================
//  OCRtoODT — STEP 3: LineTextBuilder (RAW TSV → LineTable)
//  File: src/3_LineTextBuilder/LineTextBuilder.h
//
//  Responsibility:
//      Build per-page LineTable from RAW TSV string stored in RAM.
//      This is the ONLY module that understands TSV hierarchy
//      for the OCR Text Tab (lines, gaps → empty lines).
//
//  Contract:
//      Input : Core::VirtualPage + vp.ocrTsvText (RAM)
//      Output: Tsv::LineTable* (allocated, owned by caller / page)
//      Disk  : forbidden (no I/O)
// ============================================================

#ifndef TSV_LINETEXTBUILDER_H
#define TSV_LINETEXTBUILDER_H

#include <QString>

#include "core/VirtualPage.h"
#include "3_LineTextBuilder/LineTable.h"

namespace Tsv {

class LineTextBuilder
{
public:
    // --------------------------------------------------------
    // Main API
    // --------------------------------------------------------
    static LineTable* build(const Core::VirtualPage &vp,
                            const QString          &tsvText);

private:
    // TSV parse helpers
    struct TsvRow
    {
        int     level   = -1;
        int     page    = -1;
        int     block   = -1;
        int     par     = -1;
        int     line    = -1;
        int     word    = -1;
        int     left    = 0;
        int     top     = 0;
        int     width   = 0;
        int     height  = 0;
        double  conf    = -1.0;
        QString text;
    };

    static bool parseTsvLine(const QString &line, TsvRow *out);

    static QString joinWordWithSpacing(const QString &current,
                                       const QString &nextWord);

    static int medianInt(QVector<int> v);
};

} // namespace Tsv

#endif // TSV_LINETEXTBUILDER_H
