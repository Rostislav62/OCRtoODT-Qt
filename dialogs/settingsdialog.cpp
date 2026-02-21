// ============================================================
//  OCRtoODT — Settings Dialog
//  File: dialogs/settingsdialog.cpp
//
//  Implementation of central settings dialog.
//
//  Safe Runtime Integration:
//      • RuntimePolicyManager::reapply() is called
//        only when OCR is NOT running.
//
// ============================================================

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include <QTabBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

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
#include "core/RuntimePolicyManager.h"

// ------------------------------------------------------------
// OCR controller (for safe runtime reapply check)
// ------------------------------------------------------------
#include "2_ocr/OcrPipeLineController.h"

using namespace Ocr;

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
    // Create panes
    // --------------------------------------------------------
    m_general     = new GeneralSettingsPane(ui->tabWidget);
    m_recognition = new RecognitionSettingsPane(ui->tabWidget);
    m_odt         = new ODTSettingsPane(ui->tabWidget);
    m_interface   = new InterfaceSettingsPane(ui->tabWidget);

    const bool showLogging =
        ConfigManager::instance()
            .get("ui.show_logging_tab", false)
            .toBool();

    if (showLogging)
        m_logging = new LoggingPane(ui->tabWidget);

    const bool showPreprocess =
        ConfigManager::instance()
            .get("ui.show_preprocess_tab", false)
            .toBool();

    if (showPreprocess)
        m_preproc = new PreprocessSettingsPane(ui->tabWidget);

    // --------------------------------------------------------
    // Add tabs
    // --------------------------------------------------------
    ui->tabWidget->addTab(m_general, QString());

    if (m_preproc)
        ui->tabWidget->addTab(m_preproc, QString());

    ui->tabWidget->addTab(m_recognition, QString());
    ui->tabWidget->addTab(m_odt, QString());
    ui->tabWidget->addTab(m_interface, QString());

    if (m_logging)
        ui->tabWidget->addTab(m_logging, QString());

    ui->tabWidget->tabBar()->setVisible(true);

    // --------------------------------------------------------
    // Load configuration into panes
    // --------------------------------------------------------
    loadAll();

    // --------------------------------------------------------
    // Button connections
    // --------------------------------------------------------
    connect(ui->btnOK, &QPushButton::clicked,
            this, &SettingsDialog::onOk);

    connect(ui->btnCancel, &QPushButton::clicked,
            this, &SettingsDialog::onCancel);

    connect(ui->btnResetDefaults, &QPushButton::clicked,
            this, &SettingsDialog::onResetToDefaults);

    connect(ui->btnExportConfig, &QPushButton::clicked,
            this, &SettingsDialog::onExportConfig);

    connect(ui->btnImportConfig, &QPushButton::clicked,
            this, &SettingsDialog::onImportConfig);

    // --------------------------------------------------------
    // Live UI updates (theme preview)
    // --------------------------------------------------------
    connect(m_interface,
            &InterfaceSettingsPane::uiSettingsChanged,
            ThemeManager::instance(),
            &ThemeManager::reloadFromSettings);

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
        if (!pane) return;

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
    if (m_general)     m_general->load();
    if (m_preproc)     m_preproc->load();
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
    LogRouter::instance().debug("[SettingsDialog] saveAll()");

    if (m_general)     m_general->save();
    if (m_preproc)     m_preproc->save();
    if (m_recognition) m_recognition->save();
    if (m_odt)         m_odt->save();
    if (m_interface)   m_interface->save();
    if (m_logging)     m_logging->applyToConfig();

    ConfigManager::instance().save();
}

// ============================================================
// OK handler
// ============================================================

void SettingsDialog::onOk()
{
    saveAll();
    ConfigManager::instance().reload();

    LogRouter::instance().info(
        "[SettingsDialog] Settings applied and reloaded");

    ThemeManager::instance()->applyAllFromConfig();

    // Safe runtime reapply only when OCR idle
    OcrPipelineController *ctrl =
        OcrPipelineController::instance();

    if (ctrl && !ctrl->isRunning())
        RuntimePolicyManager::reapply();

    accept();
}

// ============================================================
// Cancel
// ============================================================

void SettingsDialog::onCancel()
{
    reject();
}

// ============================================================
// Reset to defaults
// ============================================================

void SettingsDialog::onResetToDefaults()
{
    const auto reply = QMessageBox::warning(
        this,
        tr("Reset Configuration"),
        tr("Reset all settings to defaults?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply != QMessageBox::Yes)
        return;

    if (!ConfigManager::instance().resetToDefaults())
    {
        QMessageBox::critical(this,
                              tr("Reset Failed"),
                              tr("Failed to reset configuration."));
        return;
    }

    loadAll();

    QMessageBox::information(this,
                             tr("Reset Complete"),
                             tr("Configuration reset successfully."));
}

// ============================================================
// Export config
// ============================================================

void SettingsDialog::onExportConfig()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        tr("Export configuration"),
        QDir::homePath() + "/ocrtoodt_config.yaml",
        tr("YAML files (*.yaml *.yml);;All files (*.*)"));

    if (path.isEmpty())
        return;

    if (!ConfigManager::instance().exportToFile(path))
    {
        QMessageBox::critical(this,
                              tr("Export failed"),
                              tr("Failed to export configuration."));
        return;
    }

    QMessageBox::information(this,
                             tr("Export complete"),
                             tr("Configuration exported."));
}

// ============================================================
// Import config
// ============================================================

void SettingsDialog::onImportConfig()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        tr("Import configuration"),
        QDir::homePath(),
        tr("YAML files (*.yaml *.yml);;All files (*.*)"));

    if (path.isEmpty())
        return;

    if (!ConfigManager::instance().importFromFile(path))
    {
        QMessageBox::critical(this,
                              tr("Import failed"),
                              tr("Invalid configuration file."));
        return;
    }

    loadAll();

    QMessageBox::information(this,
                             tr("Import complete"),
                             tr("Configuration imported."));
}
