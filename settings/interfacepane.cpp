// ============================================================
//  OCRtoODT — Interface Settings Pane
//  File: settings/interfacepane.cpp
//
//  Responsibility:
//      Implements the "Interface" tab inside SettingsDialog.
//      Acts as an orchestration layer for:
//
//          - Appearance preview (theme, fonts, toolbar, thumbnails)
//          - Language selection UI (delegated to LanguageManager)
//          - Live preview updates without affecting MainWindow
//
//  Localization standard:
//      - Subscribes to LanguageManager::languageChanged
//      - Implements retranslate() as a single entry point
//      - Retranslates both designer UI and texts created in code
// ============================================================

#include "interfacepane.h"
#include "ui_interfacepane.h"

#include "core/ConfigManager.h"
#include "core/LanguageManager.h"
#include "core/ThemeManager.h"

#include <QApplication>
#include <QComboBox>
#include <QFileDialog>
#include <QFontComboBox>
#include <QListWidget>
#include <QMetaObject>
#include <QRadioButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QToolButton>

// ------------------------------------------------------------
// Constructor / Destructor
// ------------------------------------------------------------
InterfaceSettingsPane::InterfaceSettingsPane(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::InterfaceSettingsPane)
{
    ui->setupUi(this);

    // Subscribe to global language updates so this pane stays in sync
    connect(LanguageManager::instance(),
            &LanguageManager::languageChanged,
            this,
            &InterfaceSettingsPane::onGlobalLanguageChanged);

    // Theme mode combo is populated in code
    rebuildThemeModeComboKeepSelection();

    connect(ui->comboThemeMode,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &InterfaceSettingsPane::onThemeModeChanged);

    connect(ui->btnLoadCustomQss,
            &QPushButton::clicked,
            this,
            &InterfaceSettingsPane::onLoadCustomQss);

    // Fonts
    connect(ui->fontComboApp,
            &QFontComboBox::currentFontChanged,
            this,
            &InterfaceSettingsPane::onAppFontChanged);

    connect(ui->spinAppFontSize,
            qOverload<int>(&QSpinBox::valueChanged),
            this,
            &InterfaceSettingsPane::onAppFontSizeChanged);

    // Toolbar style
    connect(ui->radioToolbarIcons, &QRadioButton::toggled,
            this, &InterfaceSettingsPane::onToolbarStyleChanged);
    connect(ui->radioToolbarText, &QRadioButton::toggled,
            this, &InterfaceSettingsPane::onToolbarStyleChanged);
    connect(ui->radioToolbarBoth, &QRadioButton::toggled,
            this, &InterfaceSettingsPane::onToolbarStyleChanged);

    connect(ui->spinToolbarIconSize,
            qOverload<int>(&QSpinBox::valueChanged),
            this,
            &InterfaceSettingsPane::onToolbarIconSizeChanged);

    // Thumbnails
    connect(ui->spinThumbSize,
            qOverload<int>(&QSpinBox::valueChanged),
            this,
            &InterfaceSettingsPane::onThumbSizeChanged);

    // Sample page selection
    connect(ui->listPreviewPages,
            &QListWidget::currentRowChanged,
            this,
            &InterfaceSettingsPane::onPreviewPageSelected);

    // Language selector (global)
    rebuildLanguageComboKeepSelection();
    connect(ui->comboLanguage,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &InterfaceSettingsPane::onLanguageComboChanged);

    // Restore Defaults button
    connect(ui->btnRestoreDefaults,
            &QPushButton::clicked,
            this,
            [this]()
            {
                // --- Appearance ---
                ui->comboThemeMode->setCurrentIndex(1); // Light
                ui->editCustomQssPath->clear();

                // --- Language ---
                int idxLang = ui->comboLanguage->findData("en");
                if (idxLang >= 0)
                    ui->comboLanguage->setCurrentIndex(idxLang);

                // --- Fonts ---
                ui->fontComboApp->setCurrentFont(QApplication::font());
                ui->spinAppFontSize->setValue(10); // твой default

                // --- Toolbar ---
                ui->radioToolbarBoth->setChecked(true);
                ui->spinToolbarIconSize->setValue(24);

                // --- Thumbnails ---
                ui->spinThumbSize->setValue(130);

                applyPreview();
            });



    // Static preview content
    setupPreviewIcons();
    setupPreviewToolbarText();

    // Sample-based preview content
    ensureSampleContentLoaded();
    loadSamplePages();

    // Ensure texts match current translator
    retranslate();
}

InterfaceSettingsPane::~InterfaceSettingsPane()
{
    delete ui;
}

// ------------------------------------------------------------
// QWidget override
// ------------------------------------------------------------
void InterfaceSettingsPane::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateDocumentPreviewPixmap();
}

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------
void InterfaceSettingsPane::load()
{
    loadAppearance();
    loadLanguage();
    loadFonts();
    loadToolbar();
    loadThumbnails();

    applyPreview();

    // Resize-dependent update after layout settles
    QMetaObject::invokeMethod(
        this,
        &InterfaceSettingsPane::updateDocumentPreviewPixmap,
        Qt::QueuedConnection);
}

void InterfaceSettingsPane::save()
{
    saveAppearance();
    saveLanguage();
    saveFonts();
    saveToolbar();
    saveThumbnails();

    emit uiSettingsChanged();
}

void InterfaceSettingsPane::rebuildThemeModeComboKeepSelection()
{
    const int oldIndex = ui->comboThemeMode->currentIndex();
    const QSignalBlocker b(ui->comboThemeMode);

    ui->comboThemeMode->clear();
    ui->comboThemeMode->addItem(tr("Light"));
    ui->comboThemeMode->addItem(tr("Dark"));
    ui->comboThemeMode->addItem(tr("Custom"));

    if (oldIndex >= 0 && oldIndex < ui->comboThemeMode->count())
        ui->comboThemeMode->setCurrentIndex(oldIndex);
}

// ------------------------------------------------------------
// Slots (appearance / preview)
// ------------------------------------------------------------
void InterfaceSettingsPane::onThemeModeChanged(int)
{
    applyPreviewQss();
}

void InterfaceSettingsPane::onLoadCustomQss()
{
    const QString file = QFileDialog::getOpenFileName(
        this,
        tr("Select QSS file"),
        {},
        tr("QSS (*.qss)"));

    if (!file.isEmpty())
        ui->editCustomQssPath->setText(file);

    applyPreviewQss();
}

void InterfaceSettingsPane::onAppFontChanged(const QFont &)
{
    applyPreviewFonts();
}

void InterfaceSettingsPane::onAppFontSizeChanged(int)
{
    applyPreviewFonts();
}

void InterfaceSettingsPane::onToolbarStyleChanged()
{
    applyPreviewToolbar();
}

void InterfaceSettingsPane::onToolbarIconSizeChanged(int)
{
    applyPreviewToolbar();
}

void InterfaceSettingsPane::onThumbSizeChanged(int)
{
    int s = ui->spinThumbSize->value();
    if (s < 100) s = 100;
    if (s > 200) s = 200;

    if (s != ui->spinThumbSize->value())
        ui->spinThumbSize->setValue(s);

    applyPreviewThumbnails();
}


