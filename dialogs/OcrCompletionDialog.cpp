// ============================================================
//  OCRtoODT — Dialog: OCR Completion
//  File: dialogs/OcrCompletionDialog.cpp
//
//  Responsibility:
//      - Display modal notification when OCR processing completes
//      - Show theme-specific double-click demo illustration
//      - Play optional completion sound (config-driven)
//
//  Design rules:
//      - NO OCR logic
//      - NO pipeline interaction
//      - Reads configuration only via ConfigManager
//      - Theme detection via ThemeManager
//      - Fully UI-level component
//
//  UX Pattern:
//      This dialog is the reference implementation for
//      future notification dialogs in OCRtoODT.
//
// ============================================================

#include "dialogs/OcrCompletionDialog.h"
#include "ui_OcrCompletionDialog.h"

#include "core/ThemeManager.h"
#include "core/ConfigManager.h"

#include <QPixmap>
#include <QSoundEffect>
#include <QUrl>

// ============================================================
// Constructor
// ============================================================

OcrCompletionDialog::OcrCompletionDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::OcrCompletionDialog)
{
    ui->setupUi(this);

    // Apply correct illustration depending on theme
    applyThemeSpecificIllustration();

    // Play completion sound if enabled in config
    initCompletionSound();
}

// ============================================================
// Destructor
// ============================================================

OcrCompletionDialog::~OcrCompletionDialog()
{
    delete ui;
}

// ============================================================
// Select illustration based on active theme
// ============================================================

void OcrCompletionDialog::applyThemeSpecificIllustration()
{
    using ThemeMode = ThemeManager::ThemeMode;

    ThemeMode mode = ThemeManager::instance()->currentMode();

    // NOTE:
    // Auto/System are already resolved internally by ThemeManager.
    // We simply check the effective mode.
    const QString path =
        (mode == ThemeMode::Light)
            ? QStringLiteral(":/icons/icons/double_click_demo_light.svg")
            : QStringLiteral(":/icons/icons/double_click_demo_dark.svg");

    QPixmap pix(path);
    if (pix.isNull())
        return;

    ui->lblDemo->setPixmap(pix);
    ui->lblDemo->setScaledContents(true);
}

// ============================================================
// Initialize and play completion sound (config-driven)
// ============================================================

void OcrCompletionDialog::initCompletionSound()
{
    ConfigManager &cfg = ConfigManager::instance();

    // Respect user preference
    if (!cfg.get("ui.play_sound_on_finish", true).toBool())
        return;

    // Create sound effect as member to avoid premature destruction
    m_soundEffect = new QSoundEffect(this);

    const QString path =
        cfg.get("ui.sound_path", "sounds/done.wav").toString();

    m_soundEffect->setSource(QUrl(QStringLiteral("qrc:/") + path));

    const int volume =
        cfg.get("ui.sound_volume", 70).toInt();

    m_soundEffect->setVolume(static_cast<qreal>(volume) / 100.0);

    // Play asynchronously
    m_soundEffect->play();
}
