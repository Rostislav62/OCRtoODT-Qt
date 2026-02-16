// ============================================================
//  OCRtoODT — OCR Pass Configuration
//  File: src/2_ocr/OcrPassConfig.h
//
//  Responsibility:
//      Immutable configuration for a single OCR pass
//      (engine + parameters, no logic).
// ============================================================

#pragma once

#include <QString>

struct OcrPassConfig
{
    QString engine;      // "tesseract"
    QString languages;   // "rus+eng"
    int     psm;          // Tesseract PSM
    int     oem;          // Tesseract OEM
    int     dpi;          // effective DPI
    QString passName;     // e.g. "psm4", "psm3"
};
