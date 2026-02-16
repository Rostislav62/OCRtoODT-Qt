// ============================================================
//  OCRtoODT — Settings Dialog
//  File: dialogs/settingsdialog.cpp
//
//  Responsibility:
//      Implementation of the central settings dialog.
//
//      This dialog:
//          • creates and owns all settings panes
//          • synchronizes panes with ConfigManager
//          • applies runtime UI-related changes immediately
//          • updates its own localization live
// ============================================================

#include <QTabBar>

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

// ------------------------------------------------------------
// Settings panes
// ------------------------------------------------------------
#include "settings/generalpane.h"
#include "settings/preprocesspane.h"
#include "settings/recognitionpane.h"
#include "settings/odtpane.h"
#include "settings/interfacepane.h"
#include "settings/loggingpane.h"

// ------------------------------------------------------------
// Core managers
// ------------------------------------------------------------
#include "core/ConfigManager.h"
#include "core/ThemeManager.h"
#include "core/LanguageManager.h"
#include "core/LogRouter.h"

// ============================================================
// Constructor
// ============================================================

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    // --------------------------------------------------------
    // Subscribe to global language changes
    // --------------------------------------------------------
    connect(LanguageManager::instance(),
            &LanguageManager::languageChanged,
            this,
            &SettingsDialog::retranslate);

    // --------------------------------------------------------
    // Create settings panes
    // --------------------------------------------------------
    m_general     = new GeneralSettingsPane(ui->tabWidget);
    m_recognition = new RecognitionSettingsPane(ui->tabWidget);
    m_odt         = new ODTSettingsPane(ui->tabWidget);
    m_interface   = new InterfaceSettingsPane(ui->tabWidget);

    const bool showLogging =
        ConfigManager::instance()
            .get("ui.show_logging_tab", false)
            .toBool();

    if (showLogging) {
        m_logging = new LoggingPane(ui->tabWidget);
    } else {
        m_logging = nullptr;
    }


    const bool showPreprocess =
        ConfigManager::instance()
            .get("ui.show_preprocess_tab", false)
            .toBool();

    if (showPreprocess) {
        m_preproc = new PreprocessSettingsPane(ui->tabWidget);
    } else {
        m_preproc = nullptr;
    }



    // --------------------------------------------------------
    // Add panes as tabs (titles are localized in retranslate())
    // --------------------------------------------------------
    ui->tabWidget->addTab(m_general, QString());

    if (m_preproc) {
        ui->tabWidget->addTab(m_preproc, QString());
    }

    ui->tabWidget->addTab(m_recognition, QString());
    ui->tabWidget->addTab(m_odt,         QString());
    ui->tabWidget->addTab(m_interface,   QString());

    if (m_logging) {
        ui->tabWidget->addTab(m_logging, QString());
    }

    // Force tab bar visible (in case QSS hides it)
    ui->tabWidget->tabBar()->setVisible(true);

    // --------------------------------------------------------
    // Load settings into panes
    // --------------------------------------------------------
    loadAll();

    // --------------------------------------------------------
    // Buttons
    // --------------------------------------------------------
    connect(ui->btnOK,     &QPushButton::clicked,
            this,         &SettingsDialog::onOk);

    connect(ui->btnCancel, &QPushButton::clicked,
            this,          &SettingsDialog::onCancel);

    // --------------------------------------------------------
    // Apply UI-related changes live (theme, fonts, thumbnails)
    // --------------------------------------------------------
    connect(m_interface,
            &InterfaceSettingsPane::uiSettingsChanged,
            ThemeManager::instance(),
            &ThemeManager::reloadFromSettings);

    // Apply initial localization
    retranslate();
}

// ============================================================
// Destructor
// ============================================================

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

// ============================================================
// Localization
// ============================================================

void SettingsDialog::retranslate()
{
    ui->retranslateUi(this);

    auto setTitle = [&](QWidget *pane, const QString &title)
    {
        const int idx = ui->tabWidget->indexOf(pane);
        if (idx >= 0)
            ui->tabWidget->setTabText(idx, title);
    };

    setTitle(m_general,     tr("General"));
    setTitle(m_preproc,     tr("Preprocess"));
    setTitle(m_recognition, tr("Recognition"));
    setTitle(m_odt,         tr("ODT"));
    setTitle(m_interface,   tr("Interface"));
    setTitle(m_logging,     tr("Logging"));
}

// ============================================================
// Load settings into panes
// ============================================================

void SettingsDialog::loadAll()
{
    if (m_general) m_general->load();

    // Ensure preprocess pane reflects current expert mode
    const bool expert = ConfigManager::instance().get("ui.expert_mode", false).toBool();

    if (m_preproc) m_preproc->setExpertMode(expert);

    const bool showPreprocess = ConfigManager::instance().get("ui.show_preprocess_tab", false).toBool();
    if (showPreprocess ) {m_preproc->load();}

    if (m_recognition) m_recognition->load();
    if (m_odt)         m_odt->load();
    if (m_interface)   m_interface->load();
    if (m_logging)     m_logging->loadFromConfig();
}

// ============================================================
// Save panes → ConfigManager
// ============================================================

void SettingsDialog::saveAll()
{
    if (m_general)     m_general->save();
    if (m_preproc)     m_preproc->save();
    if (m_recognition) m_recognition->save();
    if (m_odt)         m_odt->save();
    if (m_interface)   m_interface->save();
    if (m_logging)     m_logging->applyToConfig();

    ConfigManager::instance().save();
}

// ============================================================
// OK button handler
// ============================================================

void SettingsDialog::onOk()
{
    // 1) Save UI state into config.yaml
    saveAll();

    // 2) Reload configuration into memory
    ConfigManager::instance().reload();

    LogRouter::instance().info(
        tr("[SettingsDialog] Settings applied and configuration reloaded"));

    // 3) Apply UI-related settings immediately
    ThemeManager::instance()->applyAllFromConfig();

    // 4) Close dialog
    accept();
}

// ============================================================
// Cancel button handler
// ============================================================

void SettingsDialog::onCancel()
{
    reject();
}
