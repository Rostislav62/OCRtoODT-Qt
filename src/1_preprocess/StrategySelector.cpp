// ============================================================
//  OCRtoODT — Preprocess Strategy Selector
//  File: 1_preprocess/strategyselector.cpp
//
//  Responsibility:
//      Convert objective image diagnostics into
//      a conservative preprocessing strategy.
//
// ============================================================

#include "1_preprocess/StrategySelector.h"

namespace Ocr {
namespace Preprocess {

PreprocessStrategy StrategySelector::select(const ImageDiagnostics &d)
{
    // 1) Binary images: do nothing
    if (d.looksBinary)
        return PreprocessStrategy::None;

    // 2) Very sharp + uniform background + low noise: do nothing
    if (d.longSidePx > 3000.0 &&
        d.blurScore > 150.0 &&
        d.backgroundVariance < 10.0 &&
        d.noiseScore < 20.0)
    {
        return PreprocessStrategy::None;
    }

    // 3) Uneven illumination but not too noisy: light cleanup (mobile)
    if (d.backgroundVariance > 30.0 &&
        d.noiseScore < 35.0 &&
        d.blurScore > 80.0)
    {
        return PreprocessStrategy::LightCleanup;
    }

    // 4) Low quality: blur OR noise OR low resolution
    if (d.blurScore < 80.0 ||
        d.noiseScore > 40.0 ||
        d.longSidePx < 1500.0)
    {
        return PreprocessStrategy::Stabilize;
    }

    return PreprocessStrategy::None;
}

const char* StrategySelector::toString(PreprocessStrategy strategy)
{
    switch (strategy) {
    case PreprocessStrategy::None:
        return "None";
    case PreprocessStrategy::LightCleanup:
        return "LightCleanup";
    case PreprocessStrategy::Stabilize:
        return "Stabilize";
    default:
        return "Unknown";
    }
}

} // namespace Preprocess
} // namespace Ocr
