// ============================================================
//  OCRtoODT — Centralized Logging Router
//  File: core/logrouter.h
//
//  Responsibility:
//      Unified logging system for the entire application.
//      ALL logs from ALL modules must pass through LogRouter.
//
//      Canonical logging model (logging.level):
//          0 — Off
//          1 — Errors only
//          2 — Warnings + Errors
//          3 — Info + Warnings + Errors
//          4 — Verbose (Debug + Performance)
//
// ============================================================

#ifndef OCRTOODT_LOGROUTER_H
#define OCRTOODT_LOGROUTER_H

#include <QObject>
#include <QMutex>
#include <QFile>
#include <QTextStream>

class LogRouter : public QObject
{
    Q_OBJECT

public:
    // --------------------------------------------------------
    // Log destinations
    // --------------------------------------------------------
    enum class Destination {
        None,
        UiOnly,
        FileOnly,
        UiAndFile,
        ConsoleOnly,
        UiFileConsole
    };
    Q_ENUM(Destination)

    // --------------------------------------------------------
    // Singleton
    // --------------------------------------------------------
    static LogRouter &instance();

    // --------------------------------------------------------
    // Logging configuration
    // --------------------------------------------------------
    void configure(bool uiEnabled,
                   bool fileEnabled,
                   bool consoleEnabled,
                   bool profilerEnabled,
                   const QString &filePath);

    void setDestination(Destination dest);

    // Canonical logging level (0..4 from config)
    void setLogLevel(int level);

    // --------------------------------------------------------
    // Log entry points (used by all modules)
    // --------------------------------------------------------
    void info(const QString &msg);
    void warning(const QString &msg);
    void error(const QString &msg);
    void perf(const QString &msg);
    void debug(const QString &msg);
    // ------------------------------------------------------------
    // Set maximum log file size (MB)
    // Called from LoggingPane + config load
    // ------------------------------------------------------------
    void setMaxLogSizeMB(int megabytes);


signals:
    // Emitted only when UI logging is enabled
    void uiMessage(const QString &msg);

private:
    explicit LogRouter(QObject *parent = nullptr);
    Q_DISABLE_COPY(LogRouter)

    // --------------------------------------------------------
    // Internal helpers
    // --------------------------------------------------------
    enum class Level {
        Error   = 1,
        Warning = 2,
        Info    = 3,
        Verbose = 4   // debug + perf
    };

    bool shouldShow(Level msgLevel) const;

    // --------------------------------------------------------
    // File output helpers
    //
    // Contract:
    //   - Rotation is checked BEFORE appending a new line.
    //   - Rotation policy keeps:
    //       <log>.1, <log>.2, <log>.3
    // --------------------------------------------------------
    void writeToFile(const QString &msg);
    void rotateIfNeeded_unlocked(qint64 incomingBytes);
    void rotateLogs_unlocked();

    void writeToConsole(const QString &msg);



    QMutex      m_mutex;
    QFile       m_logFile;
    QTextStream m_stream;

    bool m_uiEnabled       = true;
    bool m_fileEnabled     = false;
    bool m_consoleEnabled  = false;
    bool m_profilerEnabled = true;

    Destination m_destination = Destination::UiOnly;

    int m_logLevel = 3; // default: Info

    // ------------------------------------------------------------
    // Log rotation threshold (bytes)
    // Runtime-configurable (via config.yaml)
    // Default = 5 MB
    // ------------------------------------------------------------
    qint64 m_maxLogSizeBytes = 5 * 1024 * 1024;


};

#endif // OCRTOODT_LOGROUTER_H
