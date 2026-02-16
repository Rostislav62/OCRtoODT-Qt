// ============================================================
//  OCRtoODT — OCR TSV Quality Evaluation
//  File: src/2_ocr/OcrTsvQuality.h
//
//  Responsibility:
//      Analyze RAW Tesseract TSV and compute structural
//      quality metrics WITHOUT interpreting text.
//
//  RAM-first upgrade:
//      • We can analyze TSV either from a file OR directly from text.
// ============================================================

#pragma once

#include <QString>

struct OcrTsvQuality
{
    int blocks = 0;
    int paragraphs = 0;
    int lines = 0;
    int words = 0;

    double meanConf = 0.0;
    double lowConfRatio = 0.0;   // conf < 40

    bool badStructure = false;
    double score = 0.0;
};

// Disk-based (legacy)
OcrTsvQuality analyzeTsvQuality(const QString &tsvPath);

// RAM-based (new, canonical for multipass)
OcrTsvQuality analyzeTsvQualityFromText(const QString &tsvText);
