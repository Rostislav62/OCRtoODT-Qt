// ============================================================
//  OCRtoODT — Application entry point
//  File: main.cpp
//
//  Responsibility:
//      - Create QApplication
//      - Load config.yaml via ConfigManager
//      - Configure logging (LogRouter)
//      - Detect system hardware (CPU / RAM)
//      - Decide global execution mode (RAM / DISK, parallelism)
//      - Persist decisions into config.yaml
//      - Apply theme
//      - Show MainWindow
// ============================================================

#include "mainwindow.h"

#include "core/ConfigManager.h"
#include "core/ThemeManager.h"
#include "core/LanguageManager.h"
#include "core/LogRouter.h"

#include "systeminfo/systeminfo.h"

#include <QApplication>
#include <QFont>
#include <QDir>
#include <QString>

// ------------------------------------------------------------
// Helper: application base paths
// ------------------------------------------------------------
static QString applicationBaseDir()
{
    return QDir::currentPath();
}

static QString configFilePath()
{
    return applicationBaseDir() + "/config.yaml";
}

int main(int argc, char *argv[])
{
    // --------------------------------------------------------
    // Create Qt application object
    // --------------------------------------------------------
    QApplication app(argc, argv);

    // --------------------------------------------------------
    // Global application font (Linux-friendly)
    // --------------------------------------------------------
    QFont defaultFont("DejaVu Sans", 11);
    defaultFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(defaultFont);

    // --------------------------------------------------------
    // Load config.yaml
    // --------------------------------------------------------
    const QString cfgPath = configFilePath();
    ConfigManager &cfg = ConfigManager::instance();
    cfg.load(cfgPath);

    // --------------------------------------------------------
    // Configure logging (LogRouter)
    // --------------------------------------------------------
    {
        const bool loggingEnabled =
            cfg.get("logging.enabled", true).toBool();

        const int logLevel =
            cfg.get("logging.level", 3).toInt();

        const bool fileOutput =
            cfg.get("logging.file_output", false).toBool();

        const bool guiOutput =
            cfg.get("logging.gui_output", true).toBool();

        const bool consoleOutput =
            cfg.get("logging.console_output", true).toBool();

        const QString logFilePath =
            cfg.get("logging.file_path", "log/ocrtoodt.log").toString();

        // PERF / DEBUG allowed only for Verbose (level >= 4)
        const bool profilerEnabled = (logLevel >= 4);

        LogRouter &log = LogRouter::instance();

        // Каноническая конфигурация маршрутизации
        log.configure(
            guiOutput && loggingEnabled,
            fileOutput && loggingEnabled,
            consoleOutput && loggingEnabled,
            profilerEnabled,
            logFilePath
            );

        // Каноническая установка уровня
        log.setLogLevel(logLevel);
    }

    LogRouter &log = LogRouter::instance();
    log.info("OCRtoODT starting...");
    log.info(QString("Config loaded from: %1").arg(cfgPath));

    // --------------------------------------------------------
    // Detect system hardware (CPU / RAM)
    // --------------------------------------------------------
    const int cpuPhysical = si_cpu_physical_cores();
    const int cpuLogical  = si_cpu_logical_threads();
    const long long ramTotalMB = si_total_ram_mb();
    const long long ramFreeMB  = si_free_ram_mb();

    log.info("System hardware detected:");
    log.info(QString("  CPU brand          : %1").arg(si_cpu_brand_string()));
    log.info(QString("  CPU physical cores : %1").arg(cpuPhysical));
    log.info(QString("  CPU logical threads: %1").arg(cpuLogical));
    log.info(QString("  RAM total          : %1 MB").arg(ramTotalMB));
    log.info(QString("  RAM free/available : %1 MB").arg(ramFreeMB));

    // --------------------------------------------------------
    // Decide parallel execution policy
    // --------------------------------------------------------
    bool parallelEnabled = false;
    QString numProcesses = "1";

    if (cpuLogical > 1)
    {
        parallelEnabled = true;
        numProcesses = "auto";
    }

    cfg.set("general.parallel_enabled", parallelEnabled);
    cfg.set("general.num_processes", numProcesses);

    log.info(QString("Parallel execution   : %1")
                 .arg(parallelEnabled ? "enabled" : "disabled"));
    log.info(QString("Worker processes     : %1").arg(numProcesses));

    // --------------------------------------------------------
    // Decide global data mode (RAM-only vs DISK-only)
    //
    // Priority:
    //   1) Explicit value in config.yaml (ram_only / disk_only)
    //   2) "auto" or missing → decide based on free RAM
    //
    // Assumptions:
    //   - up to 50 images
    //   - up to 4 MB per image
    //   - minimum safe free RAM: 4096 MB
    // --------------------------------------------------------
    QString mode =
        cfg.get("general.mode", "auto").toString();

    if (mode == "auto")
    {
        mode = (ramFreeMB >= 4096)
        ? "ram_only"
        : "disk_only";

        // Persist auto-decision for this session
        cfg.set("general.mode", mode);
    }

    log.info(QString("Execution data mode  : %1").arg(mode));

    // --------------------------------------------------------
    // Persist updated config (decisions are fixed for session)
    // --------------------------------------------------------
    cfg.save();
    log.info("Runtime decisions saved to config.yaml");

    // --------------------------------------------------------
    // Apply global theme (after config decisions)
    // --------------------------------------------------------
    ThemeManager::instance()->applyAllFromConfig();


    // --------------------------------------------------------
    // Apply global  Language (after config decisions)
    // --------------------------------------------------------
    LanguageManager::instance()->initialize();

    // --------------------------------------------------------
    // Show main window
    // --------------------------------------------------------
    MainWindow w;
    w.show();

    log.info("Main window shown, entering event loop.");

    return app.exec();
}
