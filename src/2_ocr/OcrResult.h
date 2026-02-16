// ============================================================
//  OCRtoODT — OCR Page Result
//  File: src/2_ocr/OcrResult.h
//
//  Responsibility:
//      Transport OCR results from STEP 2.
//
//  RAM-FIRST CONTRACT:
//      • tsvText is the canonical OCR result
//      • tsvPath is OPTIONAL (debug / export only)
// ============================================================

#ifndef OCR_RESULT_H
#define OCR_RESULT_H

#include <QString>

struct OcrPageResult
{
    bool    success      = false;
    int     globalIndex  = -1;

    // --------------------------------------------------------
    // RAM result (PRIMARY)
    // --------------------------------------------------------
    QString tsvText;     // FINAL TSV in memory

    // --------------------------------------------------------
    // Optional disk artifact (debug only)
    // --------------------------------------------------------
    QString tsvPath;

    QString errorMessage;
};

#endif // OCR_RESULT_H
