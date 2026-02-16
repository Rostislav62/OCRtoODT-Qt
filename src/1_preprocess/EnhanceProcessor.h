// ============================================================
//  OCRtoODT — Preprocess: Enhance Processor 5.0 (Pure RAM Engine)
//  File: src/1_preprocess/EnhanceProcessor.h
//
//  Responsibility:
//      Perform a full preprocessing pipeline for ONE page,
//      strictly in memory, without any disk I/O or policy logic.
//
//  Key principles (v5.0):
//      - NO disk writes
//      - NO RAM/DISK decisions
//      - NO debug or save flags
//      - Pure image transformation only
//
//      All policy decisions (RAM vs DISK, debug copies, cleanup)
//      are handled exclusively by PreprocessPipeline.
//
//  Public APIs:
//      - processSingle()               : uses active profile from config
//      - processSingleWithProfile()    : analyzer-safe explicit profile
//
// ============================================================

#ifndef PREPROCESS_ENHANCEPROCESSOR_H
#define PREPROCESS_ENHANCEPROCESSOR_H

#include <QObject>
#include <QString>
#include <QHash>
#include <QImage>

#include <opencv2/core.hpp>

#include "core/VirtualPage.h"
#include "1_preprocess/PageJob.h"

namespace Ocr {
namespace Preprocess {

class EnhanceProcessor : public QObject
{
    Q_OBJECT

public:
    explicit EnhanceProcessor(QObject *parent = nullptr);

    // ------------------------------------------------------------
    // Main API — process a single page using active config profile
    // ------------------------------------------------------------
    PageJob processSingle(const Core::VirtualPage &vp,
                          int globalIndex);

    // ------------------------------------------------------------
    // Analyzer-safe API — explicit profile override
    // Does NOT mutate global config and is parallel-safe
    // ------------------------------------------------------------
    PageJob processSingleWithProfile(const Core::VirtualPage &vp,
                                     int globalIndex,
                                     const QString &profileKey);

    // ------------------------------------------------------------
    // Runtime reload (profile definitions only)
    // ------------------------------------------------------------
    void reloadActiveProfile();

private:
    // ============================================================
    // Config-driven filter parameter structs
    // ============================================================

    struct ShadowRemovalParams
    {
        bool enabled     = false;
        int  morphKernel = 31;    // odd [15..101]
    };

    struct BackgroundNormParams
    {
        bool   enabled   = false;
        double epsilon   = 0.001; // [0.0001..1.0]
        int    blurKSize = 51;    // odd [15..201]
    };

    struct GaussianParams
    {
        bool   enabled    = false;
        int    kernelSize = 3;    // odd [3..21]
        double sigma      = 0.8;  // [0.1..5.0]
    };

    struct ClaheParams
    {
        bool   enabled      = false;
        double clipLimit    = 2.0; // [1.0..10.0]
        int    tileGridSize = 8;   // [4..16]
    };

    struct SharpenParams
    {
        bool   enabled        = false;
        double strength       = 0.3; // [0.0..2.0]
        int    gaussianK      = 3;   // odd [3..21]
        double gaussianSigma = 0.8;  // [0.1..5.0]
    };

    struct AdaptiveThresholdParams
    {
        bool enabled   = false;
        int  blockSize = 31; // odd [11..101]
        int  C         = 5;  // [-20..20]
    };

    struct ProfileParams
    {
        QString name;

        ShadowRemovalParams     shadow;
        BackgroundNormParams    background;
        GaussianParams          gaussian;
        ClaheParams             clahe;
        SharpenParams           sharpen;
        AdaptiveThresholdParams adaptive;
    };

private:
    // ============================================================
    // Internal helpers — image loading & conversion
    // ============================================================

    QImage loadPageQImage(const Core::VirtualPage &vp) const;
    QImage ensureRgb888(const QImage &img) const;
    QImage resizeIfNeeded(const QImage &img,
                          bool *wasResizedDown = nullptr) const;

    cv::Mat toGrayMat(const QImage &img) const;

    // ============================================================
    // Core processing pipeline
    // ============================================================

    PageJob processSingleInternal(const Core::VirtualPage &vp,
                                  int globalIndex,
                                  const QString &profileKey);

    cv::Mat applyProfilePipeline(const cv::Mat &gray,
                                 const ProfileParams &p,
                                 bool *didEnhance) const;

    // ============================================================
    // Config reading / validation
    // ============================================================

    ProfileParams loadProfileFromConfig(const QString &profileName) const;

    QString keyFor(const QString &profileName,
                   const QString &group,
                   const QString &param) const;

    static int    clampInt(int v, int lo, int hi);
    static double clampDouble(double v, double lo, double hi);
    static int    makeOddAtLeast(int v, int minOdd);

private:
    // Cached profile parameters
    QHash<QString, ProfileParams> m_profiles;

    // Active profile name (from config)
    QString m_profile = "scanner";
};

} // namespace Preprocess
} // namespace Ocr

#endif // PREPROCESS_ENHANCEPROCESSOR_H
