// ============================================================
//  OCRtoODT — Preprocess Strategy Selector
//  File: 1_preprocess/strategyselector.h
//
//  Responsibility:
//      Decide which preprocessing STRATEGY should be used
//      based on objective image diagnostics.
//
//      IMPORTANT:
//          - Does NOT modify images
//          - Does NOT access config
//          - Does NOT know about UI
//
// ============================================================

#ifndef PREPROCESS_STRATEGYSELECTOR_H
#define PREPROCESS_STRATEGYSELECTOR_H

#include "1_preprocess/ImageAnalyzer.h"

namespace Ocr {
namespace Preprocess {

enum class PreprocessStrategy
{
    None,
    LightCleanup,
    Stabilize
};

class StrategySelector
{
public:
    static PreprocessStrategy select(const ImageDiagnostics &d);
    static const char* toString(PreprocessStrategy strategy);
};

} // namespace Preprocess
} // namespace Ocr

#endif // PREPROCESS_STRATEGYSELECTOR_H
