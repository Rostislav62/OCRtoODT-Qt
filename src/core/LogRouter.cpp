// ============================================================
//  OCRtoODT — Centralized Logging Router
//  File: core/logrouter.cpp
// ============================================================

#include "core/LogRouter.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

// ------------------------------------------------------------
// Singleton
// ------------------------------------------------------------
LogRouter &LogRouter::instance()
{
    static LogRouter inst;
    return inst;
}

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
LogRouter::LogRouter(QObject *parent)
    : QObject(parent)
{
}

// ------------------------------------------------------------
// Configure routing and open log file if needed
// ------------------------------------------------------------
void LogRouter::configure(bool uiEnabled,
                          bool fileEnabled,
                          bool consoleEnabled,
                          bool profilerEnabled,
                          const QString &filePath)
{
    QMutexLocker lock(&m_mutex);

    m_uiEnabled       = uiEnabled;
    m_fileEnabled     = fileEnabled;
    m_consoleEnabled  = consoleEnabled;
    m_profilerEnabled = profilerEnabled;

    if (!fileEnabled)
    {
        if (m_logFile.isOpen())
            m_logFile.close();
        return;
    }

    if (m_logFile.isOpen())
        m_logFile.close();

    QDir dir;
    const QString folder = QFileInfo(filePath).absolutePath();
    if (!folder.isEmpty() && !dir.exists(folder))
        dir.mkpath(folder);

    m_logFile.setFileName(filePath);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        m_stream.setDevice(&m_logFile);
        m_stream << "# OCRtoODT Log started at "
                 << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
                 << "\n";
        m_stream.flush();
    }
}

// ------------------------------------------------------------
// Destination preset (reserved for future use)
// ------------------------------------------------------------
void LogRouter::setDestination(Destination dest)
{
    QMutexLocker lock(&m_mutex);
    m_destination = dest;
}

// ------------------------------------------------------------
// Set canonical logging level (0..4)
// ------------------------------------------------------------
void LogRouter::setLogLevel(int level)
{
    QMutexLocker lock(&m_mutex);

    if (level < 0) level = 0;
    if (level > 4) level = 4;

    m_logLevel = level;
}

// ------------------------------------------------------------
// Level filter
// ------------------------------------------------------------
bool LogRouter::shouldShow(Level msgLevel) const
{
    return m_logLevel >= static_cast<int>(msgLevel);
}

// ------------------------------------------------------------
// Output helpers
// ------------------------------------------------------------
void LogRouter::writeToConsole(const QString &msg)
{
    qDebug().noquote() << msg;
}

void LogRouter::writeToFile(const QString &msg)
{
    if (!m_stream.device())
        return;

    m_stream << msg << "\n";
    m_stream.flush();
}

// ------------------------------------------------------------
// Log entry points
// ------------------------------------------------------------
void LogRouter::error(const QString &msg)
{
    QMutexLocker lock(&m_mutex);

    if (!shouldShow(Level::Error))
        return;

    const QString line = "[ERROR] " + msg;

    if (m_uiEnabled)
        emit uiMessage(line);
    if (m_fileEnabled)
        writeToFile(line);
    if (m_consoleEnabled)
        writeToConsole(line);
}

void LogRouter::warning(const QString &msg)
{
    QMutexLocker lock(&m_mutex);

    if (!shouldShow(Level::Warning))
        return;

    const QString line = "[WARN] " + msg;

    if (m_uiEnabled)
        emit uiMessage(line);
    if (m_fileEnabled)
        writeToFile(line);
    if (m_consoleEnabled)
        writeToConsole(line);
}

void LogRouter::info(const QString &msg)
{
    QMutexLocker lock(&m_mutex);

    if (!shouldShow(Level::Info))
        return;

    const QString line = "[INFO] " + msg;

    if (m_uiEnabled)
        emit uiMessage(line);
    if (m_fileEnabled)
        writeToFile(line);
    if (m_consoleEnabled)
        writeToConsole(line);
}

void LogRouter::perf(const QString &msg)
{
    QMutexLocker lock(&m_mutex);

    if (!m_profilerEnabled)
        return;

    if (!shouldShow(Level::Verbose))
        return;

    const QString line = "[PERF] " + msg;

    if (m_uiEnabled)
        emit uiMessage(line);
    if (m_fileEnabled)
        writeToFile(line);
    if (m_consoleEnabled)
        writeToConsole(line);
}

void LogRouter::debug(const QString &msg)
{
#ifdef QT_DEBUG
    QMutexLocker lock(&m_mutex);

    if (!shouldShow(Level::Verbose))
        return;

    const QString line = "[DEBUG] " + msg;

    if (m_uiEnabled)
        emit uiMessage(line);
    if (m_fileEnabled)
        writeToFile(line);
    if (m_consoleEnabled)
        writeToConsole(line);
#else
    Q_UNUSED(msg);
#endif
}
