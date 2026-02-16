// ============================================================
//  OCRtoODT — STEP 3: LineTableSerializer
//  File: src/3_LineTextBuilder/LineTableSerializer.h
//
//  Responsibility:
//      Serialize / deserialize LineTable for DEBUG and DISK_ONLY modes.
//
//  IMPORTANT:
//      - Internal project TSV format (NOT Tesseract TSV)
//      - No config access
//      - No algorithmic logic
// ============================================================

#ifndef OCRTODT_LINETABLESERIALIZER_H
#define OCRTODT_LINETABLESERIALIZER_H

#include <QString>

#include "3_LineTextBuilder/LineTable.h"

namespace Tsv {

class LineTableSerializer
{
public:
    static bool saveToTsv(const LineTable &table,
                          const QString  &filePath,
                          QString        *error = nullptr);

    static LineTable* loadFromTsv(const QString &filePath,
                                  QString       *error = nullptr);
};

} // namespace Tsv

#endif // OCRTODT_LINETABLESERIALIZER_H
