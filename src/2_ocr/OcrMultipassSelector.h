// ============================================================
//  OCRtoODT — OCR Multi-pass Selector
//  File: src/2_ocr/OcrMultipassSelector.h
//
//  Responsibility:
//      Deterministically select the best OCR pass
//      based solely on TSV structural quality metrics.
//
//  Design rules:
//      • PURE function (no I/O, no side effects)
//      • No OCR execution
//      • No config access
//      • Stable and deterministic
//
//  Used by:
//      • OcrPageWorker (STEP 2)
// ============================================================

#ifndef OCR_MULTIPASS_SELECTOR_H
#define OCR_MULTIPASS_SELECTOR_H

#include <QList>
#include <QString>

#include "2_ocr/OcrPassConfig.h"
#include "2_ocr/OcrTsvQuality.h"

namespace Ocr {

// ------------------------------------------------------------
// OCR pass result (single OCR attempt)
//
// NOTE (RAM-first):
//     tsvText is the canonical per-pass output in RAM.
//     tsvPath is intentionally kept for transitional compatibility,
//     but OcrPageWorker will no longer write per-pass temp TSV files.
// ------------------------------------------------------------
struct OcrPassResult
{
    QString        tsvText;   // RAW TSV text (sanitized), in RAM
    QString        tsvPath;   // transitional / optional (not used for per-pass anymore)

    OcrPassConfig  config;
    OcrTsvQuality  quality;
};

// ------------------------------------------------------------
// Select best OCR pass by highest quality.score
// PRECONDITION: results is non-empty
// ------------------------------------------------------------
OcrPassResult selectBestOcrPass(
    const QList<OcrPassResult> &results);

} // namespace Ocr

#endif // OCR_MULTIPASS_SELECTOR_H
