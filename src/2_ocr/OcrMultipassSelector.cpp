// ============================================================
//  OCRtoODT — OCR Multi-pass Selector
//  File: src/2_ocr/OcrMultipassSelector.cpp
//
//  Responsibility:
//      Select the best OCR pass based on TSV quality score.
//
//  Notes:
//      • Selection is purely score-based
//      • No tie-breaking heuristics beyond order stability
// ============================================================

#include "2_ocr/OcrMultipassSelector.h"

namespace Ocr {

// ------------------------------------------------------------
// Select best OCR pass by highest quality score
// ------------------------------------------------------------
OcrPassResult selectBestOcrPass(
    const QList<OcrPassResult> &results)
{
    // Defensive fallback (should not happen in normal flow)
    if (results.isEmpty())
        return OcrPassResult();

    int bestIndex = 0;
    double bestScore = results[0].quality.score;

    const int count = results.size();

    for (int i = 1; i < count; ++i)
    {
        const double score = results[i].quality.score;

        if (score > bestScore)
        {
            bestScore = score;
            bestIndex = i;
        }
    }

    return results[bestIndex];
}

} // namespace Ocr
