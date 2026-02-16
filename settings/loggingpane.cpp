// ============================================================
//  OCRtoODT — Settings: Logging Pane
//  File: settings/loggingpane.cpp
//
//  STEP 6.1.4
//  Canonical live logging synchronization
//
// ============================================================

#include "settings/loggingpane.h"
#include "ui_loggingpane.h"

#include "core/ConfigManager.h"
#include "core/LogRouter.h"

#include <QFileDialog>

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
LoggingPane::LoggingPane(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::LoggingPane)
{
    ui->setupUi(this);

    connect(ui->btnBrowse,
            &QToolButton::clicked,
            this,
            &LoggingPane::onBrowse);
}

// ------------------------------------------------------------
// Destructor
// ------------------------------------------------------------
LoggingPane::~LoggingPane()
{
    delete ui;
}

// ------------------------------------------------------------
// Load values from config.yaml → UI
// ------------------------------------------------------------
void LoggingPane::loadFromConfig()
{
    ConfigManager &cfg = ConfigManager::instance();

    const bool enabled =
        cfg.get("logging.enabled", true).toBool();

    const bool guiOutput =
        cfg.get("logging.gui_output", true).toBool();

    const bool fileOutput =
        cfg.get("logging.file_output", false).toBool();

    const bool consoleOutput =
        cfg.get("logging.console_output", true).toBool();

    const QString filePath =
        cfg.get("logging.file_path", "ocrtoodt.log").toString();

    const int level =
        cfg.get("logging.level", 3).toInt();

    ui->chkEnableLogging->setChecked(enabled);
    ui->chkGuiOutput->setChecked(guiOutput);
    ui->chkFileOutput->setChecked(fileOutput);
    ui->editFilePath->setText(filePath);

    selectLogLevel(level);

    // NOTE:
    // console_output currently has no UI checkbox by design.
    Q_UNUSED(consoleOutput);
}

// ------------------------------------------------------------
// Apply UI → ConfigManager + LogRouter (LIVE)
// ------------------------------------------------------------
void LoggingPane::applyToConfig()
{
    ConfigManager &cfg = ConfigManager::instance();

    const bool enabled      = ui->chkEnableLogging->isChecked();
    const bool guiOutput    = ui->chkGuiOutput->isChecked();
    const bool fileOutput   = ui->chkFileOutput->isChecked();
    const bool consoleOutput =
        cfg.get("logging.console_output", true).toBool();

    const QString filePath  = ui->editFilePath->text();
    const int level         = detectSelectedLogLevel();

    // --------------------------------------------------------
    // Persist to config.yaml
    // --------------------------------------------------------
    cfg.set("logging.enabled",        enabled);
    cfg.set("logging.gui_output",     guiOutput);
    cfg.set("logging.file_output",    fileOutput);
    cfg.set("logging.file_path",      filePath);
    cfg.set("logging.level",          level);

    // --------------------------------------------------------
    // LIVE apply to LogRouter
    // --------------------------------------------------------
    LogRouter &log = LogRouter::instance();

    log.setLogLevel(level);

    log.configure(
        enabled && guiOutput,      // UI output
        enabled && fileOutput,     // File output
        enabled && consoleOutput,  // Console output
        level >= 4,                // profilerEnabled
        filePath
        );

    log.info("[LoggingPane] Logging configuration updated live");
}

// ------------------------------------------------------------
// Detect selected radio → numeric logging.level
// ------------------------------------------------------------
int LoggingPane::detectSelectedLogLevel() const
{
    if (ui->rbLogOff->isChecked())             return 0;
    if (ui->rbErrorsOnly->isChecked())         return 1;
    if (ui->rbWarningsAndErrors->isChecked())  return 2;
    if (ui->rbInfoWarningsErrors->isChecked()) return 3;
    if (ui->rbVerbose->isChecked())            return 4;

    return 3;
}

// ------------------------------------------------------------
// Select radio-button by numeric level
// ------------------------------------------------------------
void LoggingPane::selectLogLevel(int level)
{
    switch (level)
    {
    case 0: ui->rbLogOff->setChecked(true); break;
    case 1: ui->rbErrorsOnly->setChecked(true); break;
    case 2: ui->rbWarningsAndErrors->setChecked(true); break;
    case 3: ui->rbInfoWarningsErrors->setChecked(true); break;
    case 4: ui->rbVerbose->setChecked(true); break;
    default:
        ui->rbInfoWarningsErrors->setChecked(true);
        break;
    }
}

// ------------------------------------------------------------
// Browse log file
// ------------------------------------------------------------
void LoggingPane::onBrowse()
{
    const QString path = QFileDialog::getSaveFileName(
        this,
        tr("Select log file"),
        ui->editFilePath->text(),
        tr("Log files (*.log);;All files (*.*)")
        );

    if (!path.isEmpty())
        ui->editFilePath->setText(path);
}
