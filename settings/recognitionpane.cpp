// ============================================================
//  OCRtoODT — Recognition Settings Pane
//  File: settings/recognitionpane.cpp
//
//  GoldenDict-style dual list architecture + Profiles UI
//
//  DESIGN RULES:
//      - No OCR logic here
//      - UI talks to OcrLanguageManager ONLY (not config)
//      - ConfigManager is used ONLY for UI settings (notify/sound)
// ============================================================

#include "recognitionpane.h"
#include "ui_recognitionpane.h"

#include "core/LanguageManager.h"
#include "core/ConfigManager.h"
#include "core/ocr/OcrLanguageManager.h"
#include "core/ocr/LanguageMeta.h"

#include <QFileDialog>
#include <QSoundEffect>
#include <QUrl>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QInputDialog>
#include <QMessageBox>
#include <QDir>
#include <QFile>

#include <algorithm>

// ============================================================
// Constructor / Destructor
// ============================================================

RecognitionSettingsPane::RecognitionSettingsPane(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RecognitionSettingsPane)
{
    ui->setupUi(this);

    // --------------------------------------------------------
    // Profiles row
    // --------------------------------------------------------
    connect(ui->comboProfiles,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &RecognitionSettingsPane::onProfileChanged);

    connect(ui->btnAddProfile,
            &QPushButton::clicked,
            this,
            &RecognitionSettingsPane::onAddProfile);

    connect(ui->btnDeleteProfile,
            &QPushButton::clicked,
            this,
            &RecognitionSettingsPane::onDeleteProfile);

    connect(ui->btnRenameProfile,
            &QPushButton::clicked,
            this,
            &RecognitionSettingsPane::onRenameProfile);

    // --------------------------------------------------------
    // Buttons between lists
    // --------------------------------------------------------
    connect(ui->btnLangAdd,
            &QPushButton::clicked,
            this,
            &RecognitionSettingsPane::onAddToActive);

    connect(ui->btnLangRemove,
            &QPushButton::clicked,
            this,
            &RecognitionSettingsPane::onRemoveFromActive);

    connect(ui->btnRefreshLanguages,
            &QPushButton::clicked,
            this,
            &RecognitionSettingsPane::onRefreshLanguages);

    connect(ui->btnAddLanguage,
            &QPushButton::clicked,
            this,
            &RecognitionSettingsPane::onInstallLanguage);

    // --------------------------------------------------------
    // Sound UI
    // --------------------------------------------------------
    connect(ui->btnTestSound,
            &QPushButton::clicked,
            this,
            &RecognitionSettingsPane::onTestSound);

    connect(ui->sliderSoundVolume,
            &QSlider::valueChanged,
            this,
            &RecognitionSettingsPane::onVolumeChanged);

    // --------------------------------------------------------
    // Auto reload if manager changes profile/languages
    // --------------------------------------------------------
    connect(&OcrLanguageManager::instance(),
            &OcrLanguageManager::languagesChanged,
            this,
            [this]()
            {
                // Profile could change, and active languages list as well
                loadProfiles();
                loadLanguages();
            });

    // --------------------------------------------------------
    // Retranslate on UI language switch
    // --------------------------------------------------------
    connect(LanguageManager::instance(),
            &LanguageManager::languageChanged,
            this,
            &RecognitionSettingsPane::retranslate);

    // --------------------------------------------------------
    // Double click behaviour
    // --------------------------------------------------------
    connect(ui->listAvailableLanguages,
            &QListWidget::itemDoubleClicked,
            this,
            [this](QListWidgetItem *item)
            {
                if (!item)
                    return;

                ui->listAvailableLanguages->setCurrentItem(item);
                onAddToActive();
            });

    connect(ui->listActiveLanguages,
            &QListWidget::itemDoubleClicked,
            this,
            [this](QListWidgetItem *item)
            {
                if (!item)
                    return;

                ui->listActiveLanguages->setCurrentItem(item);
                onRemoveFromActive();
            });
}

RecognitionSettingsPane::~RecognitionSettingsPane()
{
    delete ui;
}

// ============================================================
// Public
// ============================================================

void RecognitionSettingsPane::load()
{
    auto& mgr = OcrLanguageManager::instance();

    // --------------------------------------------------------
    // Snapshot current manager state into local pending model
    // --------------------------------------------------------
    m_pendingProfileLangs.clear();

    const QStringList profiles = mgr.profileNames();
    for (const QString& p : profiles)
        m_pendingProfileLangs.insert(p, mgr.languagesForProfile(p));

    m_pendingActiveProfile = mgr.activeProfile();

    // --------------------------------------------------------
    // Render UI from pending model
    // --------------------------------------------------------
    loadProfiles();
    loadLanguages();

    loadNotificationSettings();
    initSoundEffect();
}

bool RecognitionSettingsPane::save()
{
    auto& mgr = OcrLanguageManager::instance();

    // --------------------------------------------------------
    // Validation: active profile must have at least one language
    // --------------------------------------------------------
    const QStringList activeLangs =
        pendingLanguages(m_pendingActiveProfile);

    if (activeLangs.isEmpty())
    {
        QMessageBox::critical(
            this,
            tr("OCR"),
            tr("Active profile has no languages selected.\n"
               "Please select at least one language."));
        return false;
    }

    // --------------------------------------------------------
    // Apply pending model to storage (profiles JSON)
    // Strategy:
    //  1) Create missing profiles
    //  2) Set languages for each profile (non-empty enforced)
    //  3) Delete removed profiles
    //  4) Set active profile in config
    // --------------------------------------------------------
    const QStringList existing = mgr.profileNames();
    const QStringList pending  = m_pendingProfileLangs.keys();

    // --------------------------------------------------------
    // 1) Create missing
    // --------------------------------------------------------
    for (const QString& name : pending)
    {
        if (existing.contains(name))
            continue;

        const QStringList langs = m_pendingProfileLangs.value(name);

        if (!mgr.createProfile(name, langs))
        {
            QMessageBox::critical(
                this,
                tr("OCR"),
                tr("Failed to create profile '%1'.").arg(name));
            return false;
        }
    }

    // --------------------------------------------------------
    // 2) Update languages
    // --------------------------------------------------------
    for (const QString& name : pending)
    {
        const QStringList langs = m_pendingProfileLangs.value(name);

        if (!mgr.setLanguagesForProfile(name, langs))
        {
            QMessageBox::critical(
                this,
                tr("OCR"),
                tr("Profile '%1' has no languages selected.\n"
                   "Empty profiles are not allowed.").arg(name));
            return false;
        }
    }

    // --------------------------------------------------------
    // 3) Delete removed profiles
    // --------------------------------------------------------
    for (const QString& name : existing)
    {
        if (!pending.contains(name))
            mgr.deleteProfile(name);
    }

    // --------------------------------------------------------
    // 4) Active profile (config)
    // --------------------------------------------------------
    mgr.setActiveProfile(m_pendingActiveProfile);
    mgr.saveActiveProfile();

    // Persist UI settings
    saveNotificationSettings();

    return true;
}

// ============================================================
// UI helpers: list item <-> code mapping
// ============================================================

void RecognitionSettingsPane::addLangItemToList(QListWidget* list, const QString& code)
{
    auto& mgr = OcrLanguageManager::instance();

    // --------------------------------------------------------
    // Item text is human-readable; code stored in UserRole
    // --------------------------------------------------------
    QListWidgetItem* item = new QListWidgetItem(mgr.displayNameFor(code), list);
    item->setData(Qt::UserRole, code);
}

QString RecognitionSettingsPane::codeFromItem(const QListWidgetItem* item)
{
    if (!item)
        return {};

    return item->data(Qt::UserRole).toString().trimmed();
}

QStringList RecognitionSettingsPane::pendingLanguages(const QString& profile) const
{
    const QString key = profile.trimmed();
    if (key.isEmpty())
        return {};

    return m_pendingProfileLangs.value(key);
}

void RecognitionSettingsPane::setPendingLanguages(
    const QString& profile,
    const QStringList& langs)
{
    const QString key = profile.trimmed();
    if (key.isEmpty())
        return;

    if (!m_pendingProfileLangs.contains(key))
        return;

    m_pendingProfileLangs[key] = langs;
}

void RecognitionSettingsPane::reloadListsFromPending()
{
    loadProfiles();
    loadLanguages();
}

// ============================================================
// Profiles UI
// ============================================================

void RecognitionSettingsPane::loadProfiles()
{
    ui->comboProfiles->blockSignals(true);
    ui->comboProfiles->clear();

    const QStringList profiles = m_pendingProfileLangs.keys();

    for (const QString& name : profiles)
        ui->comboProfiles->addItem(name);

    const int idx = ui->comboProfiles->findText(m_pendingActiveProfile);
    ui->comboProfiles->setCurrentIndex(idx >= 0 ? idx : 0);

    ui->comboProfiles->blockSignals(false);

    const QString selected = ui->comboProfiles->currentText();
    const bool allowEdit = (selected != "default" && !selected.isEmpty());
    ui->btnDeleteProfile->setEnabled(allowEdit);
    ui->btnRenameProfile->setEnabled(allowEdit);
}

void RecognitionSettingsPane::onProfileChanged(int /*index*/)
{
    const QString name = ui->comboProfiles->currentText().trimmed();
    if (name.isEmpty())
        return;

    m_pendingActiveProfile = name;

    const bool allowEdit = (name != "default" && !name.isEmpty());
    ui->btnDeleteProfile->setEnabled(allowEdit);
    ui->btnRenameProfile->setEnabled(allowEdit);

    // --------------------------------------------------------
    // Reload lists for selected profile
    // --------------------------------------------------------
    loadLanguages();
}

void RecognitionSettingsPane::onAddProfile()
{
    bool ok = false;
    const QString name = QInputDialog::getText(
        this,
        tr("New profile"),
        tr("Profile name:"),
        QLineEdit::Normal,
        QString(),
        &ok);

    if (!ok)
        return;

    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty())
        return;

    if (m_pendingProfileLangs.contains(trimmed))
    {
        QMessageBox::information(
            this,
            tr("Profile"),
            tr("Profile already exists or name is invalid."));
        return;
    }

    // --------------------------------------------------------
    // Business rule (UI-level precheck):
    // New profile is created from CURRENT active list in UI.
    // Empty profile is forbidden.
    // --------------------------------------------------------
    const QStringList baseLangs =
        pendingLanguages(m_pendingActiveProfile);

    if (baseLangs.isEmpty())
    {
        QMessageBox::information(
            this,
            tr("Profile"),
            tr("No languages selected. Empty profile will not be created."));
        return;
    }

    m_pendingProfileLangs.insert(trimmed, baseLangs);

    // Make it active immediately (UX)
    m_pendingActiveProfile = trimmed;

    reloadListsFromPending();
}

void RecognitionSettingsPane::onDeleteProfile()
{
    const QString current = ui->comboProfiles->currentText().trimmed();
    if (current.isEmpty() || current == "default")
        return;

    const auto reply = QMessageBox::question(
        this,
        tr("Delete profile"),
        tr("Delete profile '%1'?").arg(current),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    m_pendingProfileLangs.remove(current);

    if (m_pendingActiveProfile == current)
        m_pendingActiveProfile = "default";

    reloadListsFromPending();
}

void RecognitionSettingsPane::onRenameProfile()
{
    auto& mgr = OcrLanguageManager::instance();

    const QString current = ui->comboProfiles->currentText().trimmed();
    if (current.isEmpty() || current == "default")
        return;

    bool ok = false;
    const QString name = QInputDialog::getText(
        this,
        tr("Rename profile"),
        tr("New profile name:"),
        QLineEdit::Normal,
        current,
        &ok);

    if (!ok)
        return;

    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty() || trimmed == current)
        return;

    // --------------------------------------------------------
    // We use manager API (atomic rename in JSON storage).
    // But we still must keep pending model consistent.
    // --------------------------------------------------------
    if (!mgr.renameProfile(current, trimmed))
    {
        QMessageBox::information(
            this,
            tr("Profile"),
            tr("Rename failed. The name may be invalid or already exists."));
        return;
    }

    // --------------------------------------------------------
    // Manager emits languagesChanged() → auto-reload will update UI.
    // Also refresh pending model to stay consistent.
    // --------------------------------------------------------
    load();
}

// ============================================================
// GoldenDict-style Language Logic
// ============================================================

void RecognitionSettingsPane::loadLanguages()
{
    auto& mgr = OcrLanguageManager::instance();

    const QStringList installed = mgr.installedLanguages();
    const QStringList active = pendingLanguages(m_pendingActiveProfile);

    ui->listAvailableLanguages->clear();
    ui->listActiveLanguages->clear();

    // --------------------------------------------------------
    // Sort Available by ISO639_1 then display name
    // --------------------------------------------------------
    auto sortKey = [&mgr](const QString& code) -> QString
    {
        const LanguageMeta meta = mgr.metaFor(code);
        const QString iso = meta.iso639_1.trimmed();
        const QString disp = mgr.displayNameFor(code).toLower();
        return QString("%1|%2|%3").arg(iso, disp, code.toLower());
    };

    // --------------------------------------------------------
    // Fill Active list (KEEP USER ORDER)
    // --------------------------------------------------------
    for (const QString& code : active)
        addLangItemToList(ui->listActiveLanguages, code);

    // --------------------------------------------------------
    // Available = installed minus active
    // --------------------------------------------------------
    QStringList availableCodes;
    for (const QString& code : installed)
    {
        if (!active.contains(code))
            availableCodes << code;
    }

    std::sort(availableCodes.begin(), availableCodes.end(),
              [&](const QString& a, const QString& b)
              {
                  return sortKey(a) < sortKey(b);
              });

    for (const QString& code : availableCodes)
        addLangItemToList(ui->listAvailableLanguages, code);
}

// ------------------------------------------------------------
// Move selected language → Active
// ------------------------------------------------------------
void RecognitionSettingsPane::onAddToActive()
{
    const auto items = ui->listAvailableLanguages->selectedItems();
    if (items.isEmpty())
        return;

    QStringList active = pendingLanguages(m_pendingActiveProfile);

    for (QListWidgetItem* item : items)
    {
        const QString code = codeFromItem(item);
        if (!code.isEmpty() && !active.contains(code))
            active << code;
    }

    setPendingLanguages(m_pendingActiveProfile, active);
    loadLanguages();
}

// ------------------------------------------------------------
// Move selected language ← Available
// ------------------------------------------------------------
void RecognitionSettingsPane::onRemoveFromActive()
{
    const auto items = ui->listActiveLanguages->selectedItems();
    if (items.isEmpty())
        return;

    QStringList active = pendingLanguages(m_pendingActiveProfile);

    for (QListWidgetItem* item : items)
    {
        const QString code = codeFromItem(item);
        if (!code.isEmpty())
            active.removeAll(code);
    }

    // --------------------------------------------------------
    // Protection: keep at least one language in active profile
    // --------------------------------------------------------
    if (active.isEmpty())
    {
        QMessageBox::information(
            this,
            tr("OCR"),
            tr("At least one language must be selected."));
        return;
    }

    setPendingLanguages(m_pendingActiveProfile, active);
    loadLanguages();
}

// ------------------------------------------------------------
// Refresh installed scan cache
// ------------------------------------------------------------
void RecognitionSettingsPane::onRefreshLanguages()
{
    OcrLanguageManager::instance().invalidateInstalledCache();
    loadLanguages();
}

// ------------------------------------------------------------
// Install language (*.traineddata) into tessdata dir
// ------------------------------------------------------------
void RecognitionSettingsPane::onInstallLanguage()
{
    const QString srcPath = QFileDialog::getOpenFileName(
        this,
        tr("Tesseract language (*.traineddata)"),
        QString(),
        tr("Tesseract language (*.traineddata)")
        );

    if (srcPath.isEmpty())
        return;

    QFileInfo fi(srcPath);
    if (!fi.exists() || fi.suffix().toLower() != "traineddata")
        return;

    auto& mgr = OcrLanguageManager::instance();

    const QString dstDir = mgr.resolvedTessdataDir();
    QDir().mkpath(dstDir);

    const QString dstPath = QDir(dstDir).filePath(fi.fileName());

    // --------------------------------------------------------
    // Copy traineddata into tessdata
    // (overwrite allowed: user is intentionally installing)
    // --------------------------------------------------------
    if (QFile::exists(dstPath))
        QFile::remove(dstPath);

    if (!QFile::copy(srcPath, dstPath))
    {
        QMessageBox::warning(
            this,
            tr("Install language"),
            tr("Failed to copy language file into tessdata:\n%1").arg(dstPath));
        return;
    }

    // Re-scan installed languages
    mgr.invalidateInstalledCache();
    loadLanguages();
}

// ============================================================
// Notifications & Sound (config-only, not OCR logic)
// ============================================================

void RecognitionSettingsPane::loadNotificationSettings()
{
    ConfigManager &cfg = ConfigManager::instance();

    ui->chkNotifyOnFinish->setChecked(
        cfg.get("ui.notify_on_finish", true).toBool());

    ui->chkSoundOnFinish->setChecked(
        cfg.get("ui.play_sound_on_finish", true).toBool());

    ui->sliderSoundVolume->setValue(
        cfg.get("ui.sound_volume", 70).toInt());
}

void RecognitionSettingsPane::saveNotificationSettings()
{
    ConfigManager &cfg = ConfigManager::instance();

    cfg.set("ui.notify_on_finish", ui->chkNotifyOnFinish->isChecked());
    cfg.set("ui.play_sound_on_finish", ui->chkSoundOnFinish->isChecked());
    cfg.set("ui.sound_volume", ui->sliderSoundVolume->value());
}

void RecognitionSettingsPane::initSoundEffect()
{
    // --------------------------------------------------------
    // Windows behaviour: shared global effect (single instance)
    // --------------------------------------------------------
    static QSoundEffect *sharedEffect = nullptr;

    if (!sharedEffect)
    {
        sharedEffect = new QSoundEffect(nullptr);

        const QString path =
            ConfigManager::instance()
                .get("ui.sound_path", "sounds/done.wav")
                .toString();

        sharedEffect->setSource(QUrl(QStringLiteral("qrc:/") + path));
    }

    m_soundEffect = sharedEffect;

    onVolumeChanged(ui->sliderSoundVolume->value());
}

void RecognitionSettingsPane::onTestSound()
{
    if (!m_soundEffect || !ui->chkSoundOnFinish->isChecked())
        return;

    m_soundEffect->stop();
    m_soundEffect->play();
}

void RecognitionSettingsPane::onVolumeChanged(int value)
{
    if (m_soundEffect)
        m_soundEffect->setVolume(static_cast<qreal>(value) / 100.0);
}

void RecognitionSettingsPane::retranslate()
{
    // --------------------------------------------------------
    // UI strings update + language labels update
    // (because displayNameFor() depends on current UI language)
    // --------------------------------------------------------
    ui->retranslateUi(this);

    // --------------------------------------------------------
    // Profiles buttons labels
    // --------------------------------------------------------
    ui->btnAddProfile->setText(tr("Add profile"));
    ui->btnDeleteProfile->setText(tr("Delete profile"));
    ui->btnRenameProfile->setText(tr("Rename profile"));

    loadProfiles();
    loadLanguages();
}
