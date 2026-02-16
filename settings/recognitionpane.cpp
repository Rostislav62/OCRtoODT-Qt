// ============================================================
//  OCRtoODT — Recognition Settings Pane
//  File: settings/recognitionpane.cpp
//
//  Responsibility:
//      - Manage OCR language selection
//      - Install new *.traineddata languages
//      - Notification and sound settings
//      - Volume control and sound test
//
//  DESIGN RULES:
//      - No OCR logic here
//      - Only config editing and UI preview
// ============================================================

#include "recognitionpane.h"
#include "ui_recognitionpane.h"

#include "core/ConfigManager.h"
#include "core/LanguageManager.h"

#include <QFileDialog>
#include <QListWidgetItem>
#include <QSoundEffect>
#include <QUrl>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

// ============================================================
// Constructor / Destructor
// ============================================================
RecognitionSettingsPane::RecognitionSettingsPane(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::RecognitionSettingsPane)
{
    ui->setupUi(this);

    connect(ui->btnRefreshLanguages, &QPushButton::clicked,
            this, &RecognitionSettingsPane::onRefreshLanguages);

    connect(ui->btnAddLanguage, &QPushButton::clicked,
            this, &RecognitionSettingsPane::onAddLanguage);

    connect(ui->listOCRLanguages, &QListWidget::itemChanged,
            this, &RecognitionSettingsPane::onLanguageSelectionChanged);

    connect(ui->btnTestSound, &QPushButton::clicked,
            this, &RecognitionSettingsPane::onTestSound);

    connect(ui->sliderSoundVolume, &QSlider::valueChanged,
            this, &RecognitionSettingsPane::onVolumeChanged);

    connect(LanguageManager::instance(),
            &LanguageManager::languageChanged,
            this,
            &RecognitionSettingsPane::retranslate);

}

RecognitionSettingsPane::~RecognitionSettingsPane()
{
    delete ui;
}

// ============================================================
// Public API
// ============================================================
void RecognitionSettingsPane::load()
{
    loadLanguages();
    loadNotificationSettings();
    initSoundEffect();
}

void RecognitionSettingsPane::save()
{
    saveLanguages();
    saveNotificationSettings();
}

// ============================================================
// OCR Languages
// ============================================================
void RecognitionSettingsPane::loadLanguages()
{
    ConfigManager &cfg = ConfigManager::instance();

    QStringList available =
        cfg.get("ocr.available_languages", "eng,rus").toString().split(',');

    QStringList selected =
        cfg.get("ocr.languages", "eng,rus").toString().split(',');

    ui->listOCRLanguages->clear();

    for (const QString &raw : available)
    {
        const QString lang = raw.trimmed();
        if (lang.isEmpty())
            continue;

        auto *item = new QListWidgetItem(lang, ui->listOCRLanguages);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(selected.contains(lang) ? Qt::Checked : Qt::Unchecked);
    }

}

void RecognitionSettingsPane::saveLanguages()
{
    ConfigManager::instance().set(
        "ocr.languages",
        getCheckedLanguages().join(',')
    );
}

QStringList RecognitionSettingsPane::getCheckedLanguages() const
{
    QStringList out;

    for (int i = 0; i < ui->listOCRLanguages->count(); ++i)
    {
        auto *item = ui->listOCRLanguages->item(i);
        if (item->checkState() == Qt::Checked)
            out << item->text().trimmed();
    }

    return out;
}

// ============================================================
// Slots
// ============================================================
void RecognitionSettingsPane::onRefreshLanguages()
{
    loadLanguages();
}

void RecognitionSettingsPane::onAddLanguage()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Tesseract language (*.traineddata)")
    );

    if (path.isEmpty())
        return;

    const QString lang = QFileInfo(path).baseName();

    auto *item = new QListWidgetItem(lang, ui->listOCRLanguages);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    item->setCheckState(Qt::Unchecked);

}

void RecognitionSettingsPane::onLanguageSelectionChanged()
{

}

// ============================================================
// Notifications & Sound
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
    if (m_soundEffect)
        return;

    m_soundEffect = new QSoundEffect(this);

    const QString path =
        ConfigManager::instance()
            .get("ui.sound_path", "sounds/done.wav")
            .toString();

    m_soundEffect->setSource(QUrl(QStringLiteral("qrc:/") + path));
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
    ui->retranslateUi(this);
}
