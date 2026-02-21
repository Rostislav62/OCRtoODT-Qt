// ============================================================
//  OCRtoODT — Application entry point
//  File: main.cpp
//
//  Responsibility:
//      - Create QApplication
//      - Resolve config.yaml path (production-safe)
//      - Load config.yaml via ConfigManager
//      - Configure logging (LogRouter)
//      - Detect system hardware (CPU / RAM)
//      - Compute effective runtime policy (UserState → RuntimeState)
//      - Publish effective runtime policy into ConfigManager (cfg.set)
//      - Apply theme (includes fonts / toolbar / thumbnails)
//      - Apply language
//      - Show MainWindow
//
//  Policy:
//      - main() MUST NOT overwrite user config.yaml automatically.
//      - Runtime auto-decisions are applied in-memory only (cfg.set),
//        so all modules observe effective values via ConfigManager.
// ============================================================

#include "mainwindow.h"

#include "core/ConfigManager.h"
#include "core/ThemeManager.h"
#include "core/LanguageManager.h"
#include "core/LogRouter.h"
#include "core/CrashHandler.h"
#include "core/ThreadPoolGuard.h"
#include "core/RuntimePolicyManager.h"


#include "systeminfo/systeminfo.h"

#include <QApplication>
#include <QCoreApplication>
#include <QFont>
#include <QDir>
#include <QString>
#include <QFileInfo>
#include <QStandardPaths>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>


// ------------------------------------------------------------
// Helper: resolve config.yaml path (PRODUCTION, deterministic)
//
// Policy:
//   - Always use user config directory on Linux:
//       ~/.config/<AppName>/config.yaml
//   - If config.yaml does not exist there, seed it from:
//       1) executable dir config.yaml (AppImage / portable)
//       2) current working dir config.yaml (dev run)
//       3) otherwise create minimal default file
// ------------------------------------------------------------
static QString resolvedConfigFilePath()
{
    // 1) Target location (user config dir)
    const QString configDir =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    QDir().mkpath(configDir);

    const QString userCfgPath = QDir(configDir).filePath("config.yaml");
    if (QFileInfo::exists(userCfgPath))
        return userCfgPath;

    // 2) Seed sources (read-only templates)
    const QString exeDir = QCoreApplication::applicationDirPath();
    const QString exeCfg = QDir(exeDir).filePath("config.yaml");

    const QString cwdCfg = QDir(QDir::currentPath()).filePath("config.yaml");

    QString seedPath;
    if (QFileInfo::exists(exeCfg))
        seedPath = exeCfg;
    else if (QFileInfo::exists(cwdCfg))
        seedPath = cwdCfg;

    // 3) If we have seed -> copy into user config dir
    if (!seedPath.isEmpty())
    {
        if (QFile::copy(seedPath, userCfgPath))
            return userCfgPath;

        // If copy failed, still return userCfgPath (we will create below)
    }

    // 4) Create minimal config.yaml if nothing else exists
    QFile f(userCfgPath);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QTextStream ts(&f);
        ts.setEncoding(QStringConverter::Utf8);

        ts << "# ============================================================\n";
        ts << "#  OCRtoODT — Configuration File (auto-created)\n";
        ts << "#  Location: " << userCfgPath << "\n";
        ts << "# ============================================================\n";
        ts << "config:\n";
        ts << "  version: 1\n";
        ts << "\n";
        ts << "logging:\n";
        ts << "  enabled: true\n";
        ts << "  level: 3\n";
        ts << "  file_output: false\n";
        ts << "  gui_output: true\n";
        ts << "  console_output: true\n";
        ts << "  file_path: log/ocrtoodt.log\n";
        ts << "\n";
        ts << "ui:\n";
        ts << "  theme_mode: dark\n";
        ts << "  custom_qss: \"\"\n";
        ts << "  app_font_family: \"\"\n";
        ts << "  app_font_size: 11\n";
        ts << "  log_font_size: 10\n";
        ts << "  toolbar_style: icons\n";
        ts << "  thumbnail_size: 160\n";
        ts << "  expert_mode: false\n";
        ts << "  notify_on_finish: true\n";
        ts << "  play_sound_on_finish: true\n";
        ts << "  sound_volume: 70\n";
        ts << "  sound_path: sounds/done.wav\n";
        ts << "\n";
        ts << "general:\n";
        ts << "  parallel_enabled: true\n";
        ts << "  num_processes: auto\n";
        ts << "  mode: auto\n";
        ts << "  debug_mode: false\n";
        ts << "  input_dir: input\n";
        ts << "  preprocess_path: preprocess\n";
        ts << "\n";
        ts << "preprocess:\n";
        ts << "  profile: scanner\n";
        ts << "\n";
        ts << "recognition:\n";
        ts << "  language: eng\n";
        ts << "  psm: 3\n";
        ts << "\n";
        ts << "odt:\n";
        ts << "  font_family: Times New Roman\n";
        ts << "  font_size: 12\n";
        ts << "  justify: true\n";
    }

    return userCfgPath;
}


// ------------------------------------------------------------
// UserState: preferences from config.yaml (persistent)
// RuntimeState: effective values for this session (non-persistent)
// ------------------------------------------------------------
struct UserState
{
    bool parallelEnabled = true;
    QString numProcesses = "auto"; // "auto" | "1" | "N" (future)
    QString dataMode     = "auto"; // "auto" | "ram_only" | "disk_only"
};

struct RuntimeState
{
    bool parallelEnabled = true;
    QString numProcesses = "auto";
    QString dataMode     = "ram_only";
};

// ------------------------------------------------------------
// Read user preferences from ConfigManager
// ------------------------------------------------------------
static UserState readUserStateFromConfig()
{
    ConfigManager &cfg = ConfigManager::instance();

    UserState u;
    u.parallelEnabled = cfg.get("general.parallel_enabled", true).toBool();
    u.numProcesses    = cfg.get("general.num_processes", "auto").toString().trimmed();
    u.dataMode        = cfg.get("general.mode", "auto").toString().trimmed();

    return u;
}

// ------------------------------------------------------------
// Compute effective runtime policy for this session
//
// Rules:
//   - If user selected explicit values => respect them.
//   - If user selected "auto" => decide based on hardware.
//   - Write effective values into ConfigManager (cfg.set) so that
//     all modules observe the same runtime policy.
//   - DO NOT save config.yaml here.
// ------------------------------------------------------------
static RuntimeState computeRuntimeState(const UserState &u,
                                        int cpuLogical,
                                        long long ramFreeMB)
{
    RuntimeState r;

    // --------------------------------------------------------
    // Parallel execution (effective)
    // --------------------------------------------------------
    r.parallelEnabled = u.parallelEnabled;

    // If machine cannot do parallelism, force sequential at runtime.
    if (cpuLogical <= 1)
        r.parallelEnabled = false;

    // Worker processes
    // NOTE: current project uses "auto" or "1".
    // If parallel disabled => "1"
    if (!r.parallelEnabled)
    {
        r.numProcesses = "1";
    }
    else
    {
        // Keep "auto" as effective choice if user uses auto.
        // If user provided a number (future), we keep it.
        r.numProcesses = u.numProcesses.isEmpty() ? "auto" : u.numProcesses;
        if (r.numProcesses != "auto")
        {
            // Minimal safety: do not allow 0 or negative.
            bool ok = false;
            const int n = r.numProcesses.toInt(&ok);
            if (!ok || n < 1)
                r.numProcesses = "auto";
        }
    }

    // --------------------------------------------------------
    // Data execution mode (effective)
    // --------------------------------------------------------
    r.dataMode = u.dataMode;

    // --------------------------------------------------------
    // Memory Guard (production safety)
    // --------------------------------------------------------
    //
    // Policy:
    //   - < 2GB free RAM → force disk_only
    //   - 2–4GB         → prefer disk_only
    //   - ≥ 4GB         → allow ram_only
    //
    // This prevents OOM / swap thrashing on large OCR batches.
    // --------------------------------------------------------

    if (ramFreeMB < 2048)
    {
        r.dataMode = "disk_only";
    }
    else if (ramFreeMB < 4096)
    {
        r.dataMode = "disk_only";
    }
    else
    {
        if (r.dataMode == "auto" || r.dataMode.isEmpty())
        {
            r.dataMode = "ram_only";
        }
    }


    return r;
}

// ------------------------------------------------------------
// Publish effective runtime policy into ConfigManager
// (in-memory only; no cfg.save())
// ------------------------------------------------------------
static void publishRuntimeStateToConfig(const RuntimeState &r)
{
    ConfigManager &cfg = ConfigManager::instance();

    cfg.set("general.parallel_enabled", r.parallelEnabled);
    cfg.set("general.num_processes", r.numProcesses);
    cfg.set("general.mode", r.dataMode);
}

int main(int argc, char *argv[])
{

    // --------------------------------------------------------
    // Create Qt application object
    // --------------------------------------------------------
    QApplication app(argc, argv);

    // --------------------------------------------------------
    // App identity (affects QStandardPaths::AppConfigLocation)
    // --------------------------------------------------------
    QCoreApplication::setOrganizationName("OCRtoODT");
    QCoreApplication::setApplicationName("OCRtoODT");


    // --------------------------------------------------------
    // Deterministic mode: always Production for release behavior
    // --------------------------------------------------------
    ConfigManager::instance().setMode(ConfigManager::Mode::Production);

    // --------------------------------------------------------
    // Minimal fallback application font (Linux-friendly)
    //
    // NOTE:
    //   ThemeManager::applyAllFromConfig() will apply final fonts
    //   from config.yaml later. This is only a safe default
    //   during early startup.
    // --------------------------------------------------------
    QFont defaultFont("DejaVu Sans", 11);
    defaultFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(defaultFont);

    // --------------------------------------------------------
    // Resolve + load config.yaml
    // --------------------------------------------------------
    const QString cfgPath = resolvedConfigFilePath();
    ConfigManager &cfg = ConfigManager::instance();

    // Temporary minimal console logger for early boot
    LogRouter::instance().configure(
        false,  // UI
        false,  // File
        true,   // Console
        false,  // Profiler
        ""      // No file
        );
    LogRouter::instance().setLogLevel(4);

    cfg.load(cfgPath);

    // ------------------------------------------------------------
    // Strict production validation
    // If config.yaml is invalid → abort application
    // ------------------------------------------------------------
    if (cfg.validationFailed())
    {
        qCritical() << "CRITICAL: config.yaml validation failed. Application will terminate.";
        return EXIT_FAILURE;
    }


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

        CrashHandler::install();

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

        const int maxSizeMB =
            cfg.get("logging.max_file_size_mb", 5).toInt();

        log.setMaxLogSizeMB(maxSizeMB);

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


    // Publish effective runtime values into ConfigManager (in-memory)
    RuntimePolicyManager::initialize(cpuLogical);


    // --------------------------------------------------------
    // Apply global theme (after effective config decisions)
    // NOTE: ThemeManager applies fonts too (from config keys).
    // --------------------------------------------------------
    ThemeManager::instance()->applyAllFromConfig();

    // --------------------------------------------------------
    // Apply global language (after effective config decisions)
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
