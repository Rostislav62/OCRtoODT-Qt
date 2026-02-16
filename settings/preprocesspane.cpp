// ============================================================
//  OCRtoODT — Settings: Preprocess Pane
//  File: settings/preprocesspane.cpp
//
//  Responsibility:
//      - UI for preprocessing profile selection
//      - Description of selected profile
//      - Advanced (professional) preprocessing parameters
//      - Read/write values from/to config.yaml
//      - Provide "Reset current profile to defaults" action
//        via an extra combo item (no extra buttons)
//
//  STEP 3 (Protection / Recoverability):
//      - In Expert mode, destructive reset requires confirmation.
//      - Default profiles are stored in a single table-driven block.
//
//  IMPORTANT DESIGN RULES:
//      - This pane NEVER applies image processing itself
//      - This pane NEVER reloads pipeline
//      - It only edits config values
// ============================================================

#include "settings/preprocesspane.h"
#include "ui_preprocesspane.h"

#include "core/ConfigManager.h"
#include "core/LanguageManager.h"


#include <QSignalBlocker>
#include <QMessageBox>
#include <QMap>

    // ============================================================
    // Combo index layout (must match preprocesspane.ui items):
    //   0 - mobile
    //   1 - scanner
    //   2 - low_quality
    //   3 - pdf_auto
    //   4 - reset_to_defaults (virtual action)
    // ============================================================
static constexpr int kProfileMobileIndex      = 0;
static constexpr int kProfileScannerIndex     = 1;
static constexpr int kProfileLowQualityIndex  = 2;
static constexpr int kProfilePdfAutoIndex     = 3;
static constexpr int kResetToDefaultsIndex    = 4;

// ============================================================
// Map combo index -> profile key (0..3 only)
// ============================================================
static QString profileKeyFromIndex(int index)
{
    switch (index) {
    case kProfileMobileIndex:     return "mobile";
    case kProfileScannerIndex:    return "scanner";
    case kProfileLowQualityIndex: return "low_quality";
    default:                      return "pdf_auto";
    }
}

// ============================================================
// Human-readable profile name for UI / dialogs
// ============================================================
static QString profileDisplayName(const QString &profileKey)
{
    if (profileKey == "mobile")
        return QObject::tr("Photo from mobile phone");

    if (profileKey == "scanner")
        return QObject::tr("Scanner image");

    if (profileKey == "low_quality")
        return QObject::tr("Low quality");

    if (profileKey == "pdf_auto")
        return QObject::tr("Automatic analyzer");

    return profileKey; // fallback (should never happen)
}


// ============================================================
// Default profile table (single source of truth)
// ------------------------------------------------------------
// We store key/value pairs relative to:
//   preprocess.profiles.<profileKey>.<suffix>
//
// Example suffix:
//   "gaussian_blur.enabled"
//   "gaussian_blur.kernel_size"
//   "adaptive_threshold.C"
// ============================================================
using KV = QMap<QString, QVariant>;

static const QMap<QString, KV> &defaultProfiles()
{
    static const QMap<QString, KV> kDefaults = {
        {
            "mobile",
            KV{
               {"shadow_removal.enabled", true},
               {"shadow_removal.morph_kernel", 31},

               {"background_normalization.enabled", true},
               {"background_normalization.blur_ksize", 51},
               {"background_normalization.epsilon", 0.001},

               {"gaussian_blur.enabled", true},
               {"gaussian_blur.kernel_size", 5},
               {"gaussian_blur.sigma", 1.0},

               {"clahe.enabled", true},
               {"clahe.clip_limit", 2.0},
               {"clahe.tile_grid_size", 8},

               {"sharpen.enabled", true},
               {"sharpen.strength", 0.5},
               {"sharpen.gaussian_ksize", 3},
               {"sharpen.gaussian_sigma", 0.8},

               {"adaptive_threshold.enabled", true},
               {"adaptive_threshold.block_size", 31},
               {"adaptive_threshold.C", 5},

               {"sauvola.enabled", false},
               {"sauvola.window_size", 31},
               {"sauvola.k", 0.34},
               {"sauvola.R", 128},
               }
        },
        {
            "scanner",
            KV{
               {"shadow_removal.enabled", false},
               {"shadow_removal.morph_kernel", 31},

               {"background_normalization.enabled", true},
               {"background_normalization.blur_ksize", 101},
               {"background_normalization.epsilon", 0.01},

               {"gaussian_blur.enabled", true},
               {"gaussian_blur.kernel_size", 3},
               {"gaussian_blur.sigma", 0.5},

               {"clahe.enabled", false},
               {"clahe.clip_limit", 2.0},
               {"clahe.tile_grid_size", 8},

               {"sharpen.enabled", false},
               {"sharpen.strength", 0.8},
               {"sharpen.gaussian_ksize", 3},
               {"sharpen.gaussian_sigma", 0.8},

               {"adaptive_threshold.enabled", false},
               {"adaptive_threshold.block_size", 31},
               {"adaptive_threshold.C", 5},

               {"sauvola.enabled", false},
               {"sauvola.window_size", 31},
               {"sauvola.k", 0.34},
               {"sauvola.R", 128},
               }
        },
        {
            "low_quality",
            KV{
               {"shadow_removal.enabled", false},
               {"shadow_removal.morph_kernel", 31},

               {"background_normalization.enabled", true},
               {"background_normalization.blur_ksize", 51},
               {"background_normalization.epsilon", 0.001},

               {"gaussian_blur.enabled", true},
               {"gaussian_blur.kernel_size", 7},
               {"gaussian_blur.sigma", 1.5},

               {"clahe.enabled", true},
               {"clahe.clip_limit", 3.0},
               {"clahe.tile_grid_size", 8},

               {"sharpen.enabled", true},
               {"sharpen.strength", 0.3},
               {"sharpen.gaussian_ksize", 3},
               {"sharpen.gaussian_sigma", 0.8},

               {"adaptive_threshold.enabled", false},
               {"adaptive_threshold.block_size", 31},
               {"adaptive_threshold.C", 5},

               {"sauvola.enabled", false},
               {"sauvola.window_size", 31},
               {"sauvola.k", 0.34},
               {"sauvola.R", 128},
               }
        },
        {
            "pdf_auto",
            KV{
               {"shadow_removal.enabled", false},
               {"shadow_removal.morph_kernel", 31},

               {"background_normalization.enabled", false},
               {"background_normalization.blur_ksize", 51},
               {"background_normalization.epsilon", 0.001},

               {"gaussian_blur.enabled", true},
               {"gaussian_blur.kernel_size", 3},
               {"gaussian_blur.sigma", 0.8},

               {"clahe.enabled", false},
               {"clahe.clip_limit", 2.0},
               {"clahe.tile_grid_size", 8},

               {"sharpen.enabled", false},
               {"sharpen.strength", 0.8},
               {"sharpen.gaussian_ksize", 3},
               {"sharpen.gaussian_sigma", 0.8},

               {"adaptive_threshold.enabled", false},
               {"adaptive_threshold.block_size", 31},
               {"adaptive_threshold.C", 5},

               {"sauvola.enabled", false},
               {"sauvola.window_size", 31},
               {"sauvola.k", 0.34},
               {"sauvola.R", 128},
               }
        }
    };

    return kDefaults;
}

// ============================================================
// Constructor
// ============================================================
PreprocessSettingsPane::PreprocessSettingsPane(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::PreprocessSettingsPane)
{
    ui->setupUi(this);

    connect(LanguageManager::instance(),
            &LanguageManager::languageChanged,
            this,
            &PreprocessSettingsPane::retranslate);


    ui->lblPreprocessDescription->clear();

    connect(ui->comboPreprocessProfile,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &PreprocessSettingsPane::onProfileChanged);
}

// ============================================================
// Destructor
// ============================================================
PreprocessSettingsPane::~PreprocessSettingsPane()
{
    delete ui;
}

// ============================================================
// Expert mode toggle
// ============================================================
void PreprocessSettingsPane::setExpertMode(bool enabled)
{
    if (!ui)
        return;

    m_expertMode = enabled;
    ui->groupAdvanced->setVisible(enabled);
}

// ============================================================
// Normalize profile index:
// - If "reset" item is selected, treat it as "last real profile"
// ============================================================
int PreprocessSettingsPane::normalizedProfileIndex(int index) const
{
    if (index == kResetToDefaultsIndex)
        return m_lastProfileIndex;

    if (index < kProfileMobileIndex)
        return kProfileMobileIndex;

    if (index > kProfilePdfAutoIndex)
        return kProfilePdfAutoIndex;

    return index;
}

// ============================================================
// Load settings from config.yaml → UI
// ============================================================
void PreprocessSettingsPane::load()
{
    if (!ui)
        return;

    ConfigManager &cfg = ConfigManager::instance();

    const QString profile = cfg.get("preprocess.profile", "scanner").toString();

    {
        const QSignalBlocker blocker(ui->comboPreprocessProfile);

        if (profile == "mobile")
            ui->comboPreprocessProfile->setCurrentIndex(kProfileMobileIndex);
        else if (profile == "scanner")
            ui->comboPreprocessProfile->setCurrentIndex(kProfileScannerIndex);
        else if (profile == "low_quality")
            ui->comboPreprocessProfile->setCurrentIndex(kProfileLowQualityIndex);
        else
            ui->comboPreprocessProfile->setCurrentIndex(kProfilePdfAutoIndex);
    }

    m_lastProfileIndex = ui->comboPreprocessProfile->currentIndex();
    if (m_lastProfileIndex == kResetToDefaultsIndex)
        m_lastProfileIndex = kProfileScannerIndex;

    const QString base = "preprocess.profiles." + profile + ".";

    ui->chkShadowEnabled->setChecked(
        cfg.get(base + "shadow_removal.enabled", false).toBool());
    ui->spinShadowMorphKernel->setValue(
        cfg.get(base + "shadow_removal.morph_kernel", 31).toInt());

    ui->chkBgEnabled->setChecked(
        cfg.get(base + "background_normalization.enabled", false).toBool());
    ui->spinBgBlurKsize->setValue(
        cfg.get(base + "background_normalization.blur_ksize", 51).toInt());
    ui->dblBgEpsilon->setValue(
        cfg.get(base + "background_normalization.epsilon", 0.001).toDouble());

    ui->chkGaussEnabled->setChecked(
        cfg.get(base + "gaussian_blur.enabled", false).toBool());
    ui->spinGaussKernel->setValue(
        cfg.get(base + "gaussian_blur.kernel_size", 3).toInt());
    ui->dblGaussSigma->setValue(
        cfg.get(base + "gaussian_blur.sigma", 0.8).toDouble());

    ui->chkClaheEnabled->setChecked(
        cfg.get(base + "clahe.enabled", false).toBool());
    ui->dblClaheClipLimit->setValue(
        cfg.get(base + "clahe.clip_limit", 2.0).toDouble());
    ui->spinClaheTileGrid->setValue(
        cfg.get(base + "clahe.tile_grid_size", 8).toInt());

    ui->chkSharpenEnabled->setChecked(
        cfg.get(base + "sharpen.enabled", false).toBool());
    ui->dblSharpenStrength->setValue(
        cfg.get(base + "sharpen.strength", 0.3).toDouble());
    ui->spinSharpenGaussianK->setValue(
        cfg.get(base + "sharpen.gaussian_ksize", 3).toInt());
    ui->dblSharpenGaussianSigma->setValue(
        cfg.get(base + "sharpen.gaussian_sigma", 0.8).toDouble());

    ui->chkAdaptiveEnabled->setChecked(
        cfg.get(base + "adaptive_threshold.enabled", false).toBool());
    ui->spinAdaptiveBlockSize->setValue(
        cfg.get(base + "adaptive_threshold.block_size", 31).toInt());
    ui->spinAdaptiveC->setValue(
        cfg.get(base + "adaptive_threshold.C", 5).toInt());

    updateDescription(ui->comboPreprocessProfile->currentIndex());
}

// ============================================================
// Save UI → ConfigManager (persist happens elsewhere on OK)
// ============================================================
void PreprocessSettingsPane::save()
{
    if (!ui)
        return;

    ConfigManager &cfg = ConfigManager::instance();

    const int idx = normalizedProfileIndex(ui->comboPreprocessProfile->currentIndex());
    const QString profile = profileKeyFromIndex(idx);

    cfg.set("preprocess.profile", profile);

    const QString base = "preprocess.profiles." + profile + ".";

    cfg.set(base + "shadow_removal.enabled", ui->chkShadowEnabled->isChecked());
    cfg.set(base + "shadow_removal.morph_kernel", ui->spinShadowMorphKernel->value());

    cfg.set(base + "background_normalization.enabled", ui->chkBgEnabled->isChecked());
    cfg.set(base + "background_normalization.blur_ksize", ui->spinBgBlurKsize->value());
    cfg.set(base + "background_normalization.epsilon", ui->dblBgEpsilon->value());

    cfg.set(base + "gaussian_blur.enabled", ui->chkGaussEnabled->isChecked());
    cfg.set(base + "gaussian_blur.kernel_size", ui->spinGaussKernel->value());
    cfg.set(base + "gaussian_blur.sigma", ui->dblGaussSigma->value());

    cfg.set(base + "clahe.enabled", ui->chkClaheEnabled->isChecked());
    cfg.set(base + "clahe.clip_limit", ui->dblClaheClipLimit->value());
    cfg.set(base + "clahe.tile_grid_size", ui->spinClaheTileGrid->value());

    cfg.set(base + "sharpen.enabled", ui->chkSharpenEnabled->isChecked());
    cfg.set(base + "sharpen.strength", ui->dblSharpenStrength->value());
    cfg.set(base + "sharpen.gaussian_ksize", ui->spinSharpenGaussianK->value());
    cfg.set(base + "sharpen.gaussian_sigma", ui->dblSharpenGaussianSigma->value());

    cfg.set(base + "adaptive_threshold.enabled", ui->chkAdaptiveEnabled->isChecked());
    cfg.set(base + "adaptive_threshold.block_size", ui->spinAdaptiveBlockSize->value());
    cfg.set(base + "adaptive_threshold.C", ui->spinAdaptiveC->value());
}

// ============================================================
// Confirm reset (Expert mode only)
// ============================================================
bool PreprocessSettingsPane::confirmResetInExpertMode(const QString &profileKey)
{
    if (!m_expertMode)
        return true;

    const QString title = tr("Reset profile");
    const QString profileName = profileDisplayName(profileKey);

    const QString text =
        tr("Reset profile \"%1\" to default values?\n\n"
           "All changes made in expert mode for this profile will be lost.")
            .arg(profileName);

    const auto res = QMessageBox::question(
        this,
        title,
        text,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    return (res == QMessageBox::Yes);
}

// ============================================================
// Apply defaults from table to config for one profile
// ============================================================
void PreprocessSettingsPane::resetProfileToDefaults(const QString &profileKey)
{
    ConfigManager &cfg = ConfigManager::instance();

    const auto &all = defaultProfiles();
    if (!all.contains(profileKey))
        return;

    const QString base = "preprocess.profiles." + profileKey + ".";

    const KV &kv = all[profileKey];
    for (auto it = kv.begin(); it != kv.end(); ++it)
        cfg.set(base + it.key(), it.value());
}

// ============================================================
// Profile changed handler
// ============================================================
void PreprocessSettingsPane::onProfileChanged(int index)
{
    if (!ui)
        return;

    // --------------------------------------------------------
    // Special case: "reset to defaults" virtual action
    // --------------------------------------------------------
    if (index == kResetToDefaultsIndex)
    {
        const int realIdx = normalizedProfileIndex(index);
        const QString profileKey = profileKeyFromIndex(realIdx);

        // Expert-mode confirmation
        if (!confirmResetInExpertMode(profileKey))
        {
            // User cancelled → restore selection back to real profile
            const QSignalBlocker blocker(ui->comboPreprocessProfile);
            ui->comboPreprocessProfile->setCurrentIndex(realIdx);
            updateDescription(realIdx);
            return;
        }

        // Apply defaults to the profile that was selected BEFORE reset
        resetProfileToDefaults(profileKey);

        // Keep active profile unchanged
        ConfigManager::instance().set("preprocess.profile", profileKey);

        // Return combo selection back to the real profile (no recursion)
        {
            const QSignalBlocker blocker(ui->comboPreprocessProfile);
            ui->comboPreprocessProfile->setCurrentIndex(realIdx);
        }

        // Reload UI from config (now containing defaults)
        load();
        return;
    }

    // --------------------------------------------------------
    // Normal profile selection
    // --------------------------------------------------------
    m_lastProfileIndex = normalizedProfileIndex(index);

    updateDescription(index);

    const QString profile = profileKeyFromIndex(m_lastProfileIndex);
    ConfigManager::instance().set("preprocess.profile", profile);

    load();
}

// ============================================================
// Description label helper
// ============================================================
void PreprocessSettingsPane::updateDescription(int index)
{
    if (index == kResetToDefaultsIndex) {
        ui->lblPreprocessDescription->setText(
            tr("Reset current profile to default values."));
        return;
    }

    switch (index) {
    case kProfileMobileIndex:
        ui->lblPreprocessDescription->setText(
            tr("Photo from a mobile phone — uneven lighting, shadows, requires more aggressive preprocessing."));
        break;
    case kProfileScannerIndex:
        ui->lblPreprocessDescription->setText(
            tr("Scanner image — minimal processing without unnecessary distortions. Recommended as default."));
        break;
    case kProfileLowQualityIndex:
        ui->lblPreprocessDescription->setText(
            tr("Low quality — noise, blurred characters, old books. May require stronger filtering."));
        break;
    default:
        ui->lblPreprocessDescription->setText(
            tr("Automatic analyzer — profile is determined automatically based on source and quality."));
        break;
    }
}


// ============================================================
// Retranslate UI after language change
// ============================================================
void PreprocessSettingsPane::retranslate()
{
    ui->retranslateUi(this);

    // Update dynamic description text for current profile
    updateDescription(ui->comboPreprocessProfile->currentIndex());
}
