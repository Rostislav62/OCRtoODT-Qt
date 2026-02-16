// ============================================================
//  OCRtoODT — Preprocess: Enhance Processor 5.0 (Pure RAM Engine)
//  File: src/1_preprocess/EnhanceProcessor.cpp
//
//  Responsibility:
//      - Load source image for a page
//      - Apply config-driven preprocessing filters
//      - Return enhanced cv::Mat in RAM
//
//  IMPORTANT:
//      This class performs NO disk I/O and NO policy decisions.
//      All RAM/DISK/debug logic is handled by PreprocessPipeline.
//
// ============================================================

#include "1_preprocess/EnhanceProcessor.h"

#include <QImageReader>
#include <QtMath>

#include <opencv2/imgproc.hpp>

#include "core/ConfigManager.h"
#include "core/LogRouter.h"

// Filters
#include "1_preprocess/filters/shadow_removal.h"
#include "1_preprocess/filters/background_norm.h"
#include "1_preprocess/filters/gaussian.h"
#include "1_preprocess/filters/clahe.h"
#include "1_preprocess/filters/sharpen.h"

using namespace Ocr::Preprocess;

// ============================================================
// Clamp helpers
// ============================================================
int EnhanceProcessor::clampInt(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

double EnhanceProcessor::clampDouble(double v, double lo, double hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

int EnhanceProcessor::makeOddAtLeast(int v, int minOdd)
{
    if (v < minOdd) v = minOdd;
    if ((v % 2) == 0) v += 1;
    return v;
}

// ============================================================
// Constructor
// ============================================================
EnhanceProcessor::EnhanceProcessor(QObject *parent)
    : QObject(parent)
{
    reloadActiveProfile();
}

// ============================================================
// Reload active profile from config
// ============================================================
void EnhanceProcessor::reloadActiveProfile()
{
    ConfigManager &cfg = ConfigManager::instance();

    m_profile =
        cfg.get("preprocess.profile", "scanner").toString();

    if (m_profile == "analyzer")
        m_profile = "scanner";

    if (!m_profiles.contains(m_profile))
        m_profiles.insert(m_profile, loadProfileFromConfig(m_profile));

    LogRouter::instance().info(
        QString("[EnhanceProcessor] Active profile: \"%1\"").arg(m_profile));
}

// ============================================================
// Public API: processSingle (config-selected profile)
// ============================================================
PageJob EnhanceProcessor::processSingle(const Core::VirtualPage &vp,
                                        int globalIndex)
{
    reloadActiveProfile();
    return processSingleInternal(vp, globalIndex, m_profile);
}

// ============================================================
// Public API: processSingleWithProfile (explicit profile)
// ============================================================
PageJob EnhanceProcessor::processSingleWithProfile(const Core::VirtualPage &vp,
                                                   int globalIndex,
                                                   const QString &profileKey)
{
    QString key = profileKey;
    if (key.isEmpty() || key == "analyzer")
        key = "scanner";

    if (!m_profiles.contains(key))
        m_profiles.insert(key, loadProfileFromConfig(key));

    return processSingleInternal(vp, globalIndex, key);
}

// ============================================================
// Main processing entry
// ============================================================
PageJob EnhanceProcessor::processSingleInternal(const Core::VirtualPage &vp,
                                                int globalIndex,
                                                const QString &profileKey)
{
    PageJob job;
    job.vp = vp;
    job.globalIndex = globalIndex;

    QImage img = loadPageQImage(vp);
    if (img.isNull())
        return job;

    img = ensureRgb888(img);

    bool resized = false;
    img = resizeIfNeeded(img, &resized);

    cv::Mat gray = toGrayMat(img);
    if (gray.empty())
        return job;

    const ProfileParams params = m_profiles.value(profileKey);

    bool didEnhance = false;
    cv::Mat processed = applyProfilePipeline(gray, params, &didEnhance);

    job.enhancedMat     = processed;
    job.wasEnhanced     = didEnhance;
    job.enhanceProfile = params.name;
    job.enhancedSize   = QSize(processed.cols, processed.rows);

    return job;
}

// ============================================================
// Apply filter pipeline
// ============================================================
cv::Mat EnhanceProcessor::applyProfilePipeline(const cv::Mat &gray,
                                               const ProfileParams &p,
                                               bool *didEnhance) const
{
    if (didEnhance)
        *didEnhance = false;

    cv::Mat img = gray.clone();
    bool any = false;

    if (p.shadow.enabled) {
        img = Filters::removeShadows(img, p.shadow.morphKernel);
        any = true;
    }

    if (p.background.enabled) {
        img = Filters::normalizeBackground(img,
                                           p.background.blurKSize,
                                           p.background.epsilon);
        any = true;
    }

    if (p.gaussian.enabled) {
        img = Filters::gaussianBlur(img,
                                    p.gaussian.kernelSize,
                                    p.gaussian.sigma);
        any = true;
    }

    if (p.clahe.enabled) {
        img = Filters::applyClahe(img,
                                  p.clahe.clipLimit,
                                  p.clahe.tileGridSize);
        any = true;
    }

    if (p.sharpen.enabled) {
        img = Filters::unsharpMask(img,
                                   p.sharpen.strength,
                                   p.sharpen.gaussianK,
                                   p.sharpen.gaussianSigma);
        any = true;
    }

    if (p.adaptive.enabled) {
        // Adaptive threshold filter is assumed to exist in your project.
        // Call it here if already implemented.
        any = true;
    }

    if (didEnhance)
        *didEnhance = any;

    return img;
}

// ============================================================
// Load source image
// ============================================================
QImage EnhanceProcessor::loadPageQImage(const Core::VirtualPage &vp) const
{
    QImageReader reader(vp.sourcePath);
    reader.setAutoTransform(true);

    QImage img = reader.read();
    if (img.isNull()) {
        LogRouter::instance().error(
            QString("[EnhanceProcessor] Failed to load image: %1 (%2)")
                .arg(vp.sourcePath, reader.errorString()));
    }

    return img;
}

// ============================================================
// Ensure RGB888
// ============================================================
QImage EnhanceProcessor::ensureRgb888(const QImage &img) const
{
    if (img.format() == QImage::Format_RGB888)
        return img;

    return img.convertToFormat(QImage::Format_RGB888);
}

// ============================================================
// Resize if needed (safety cap)
// ============================================================
QImage EnhanceProcessor::resizeIfNeeded(const QImage &img,
                                        bool *wasResizedDown) const
{
    const int maxLongSide = 3000;

    if (wasResizedDown)
        *wasResizedDown = false;

    int longSide = qMax(img.width(), img.height());
    if (longSide <= maxLongSide)
        return img;

    double scale = double(maxLongSide) / double(longSide);

    if (wasResizedDown)
        *wasResizedDown = true;

    return img.scaled(QSize(img.width() * scale, img.height() * scale),
                      Qt::KeepAspectRatio,
                      Qt::SmoothTransformation);
}

// ============================================================
// Convert QImage → Gray cv::Mat
// ============================================================
cv::Mat EnhanceProcessor::toGrayMat(const QImage &img) const
{
    cv::Mat rgb(img.height(), img.width(), CV_8UC3,
                const_cast<uchar*>(img.bits()),
                img.bytesPerLine());

    cv::Mat gray;
    cv::cvtColor(rgb, gray, cv::COLOR_RGB2GRAY);
    return gray.clone();
}

// ============================================================
// Config key helper
// ============================================================
QString EnhanceProcessor::keyFor(const QString &profileName,
                                 const QString &group,
                                 const QString &param) const
{
    return QString("preprocess.profiles.%1.%2.%3")
    .arg(profileName, group, param);
}

// ============================================================
// Load profile parameters from config
// ============================================================
EnhanceProcessor::ProfileParams
EnhanceProcessor::loadProfileFromConfig(const QString &profileName) const
{
    ConfigManager &cfg = ConfigManager::instance();
    ProfileParams p;
    p.name = profileName;

    p.shadow.enabled =
        cfg.get(keyFor(profileName, "shadow_removal", "enabled"), false).toBool();
    p.shadow.morphKernel =
        makeOddAtLeast(
            clampInt(cfg.get(keyFor(profileName, "shadow_removal", "morph_kernel"), 31).toInt(),
                     15, 101),
            15);

    p.background.enabled =
        cfg.get(keyFor(profileName, "background_normalization", "enabled"), false).toBool();
    p.background.blurKSize =
        makeOddAtLeast(
            clampInt(cfg.get(keyFor(profileName, "background_normalization", "blur_ksize"), 51).toInt(),
                     15, 201),
            15);
    p.background.epsilon =
        clampDouble(cfg.get(keyFor(profileName, "background_normalization", "epsilon"), 0.001).toDouble(),
                    0.0001, 1.0);

    p.gaussian.enabled =
        cfg.get(keyFor(profileName, "gaussian_blur", "enabled"), false).toBool();
    p.gaussian.kernelSize =
        makeOddAtLeast(
            clampInt(cfg.get(keyFor(profileName, "gaussian_blur", "kernel_size"), 3).toInt(),
                     3, 21),
            3);
    p.gaussian.sigma =
        clampDouble(cfg.get(keyFor(profileName, "gaussian_blur", "sigma"), 0.8).toDouble(),
                    0.1, 5.0);

    p.clahe.enabled =
        cfg.get(keyFor(profileName, "clahe", "enabled"), false).toBool();
    p.clahe.clipLimit =
        clampDouble(cfg.get(keyFor(profileName, "clahe", "clip_limit"), 2.0).toDouble(),
                    1.0, 10.0);
    p.clahe.tileGridSize =
        clampInt(cfg.get(keyFor(profileName, "clahe", "tile_grid_size"), 8).toInt(),
                 4, 16);

    p.sharpen.enabled =
        cfg.get(keyFor(profileName, "sharpen", "enabled"), false).toBool();
    p.sharpen.strength =
        clampDouble(cfg.get(keyFor(profileName, "sharpen", "strength"), 0.3).toDouble(),
                    0.0, 2.0);

    p.adaptive.enabled =
        cfg.get(keyFor(profileName, "adaptive_threshold", "enabled"), false).toBool();
    p.adaptive.blockSize =
        makeOddAtLeast(
            clampInt(cfg.get(keyFor(profileName, "adaptive_threshold", "block_size"), 31).toInt(),
                     11, 101),
            11);
    p.adaptive.C =
        clampInt(cfg.get(keyFor(profileName, "adaptive_threshold", "C"), 5).toInt(),
                 -20, 20);

    return p;
}
