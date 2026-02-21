// ============================================================
//  OCRtoODT — Centralized Logging Router
//  File: core/logrouter.cpp
// ============================================================

#include "core/LogRouter.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFile>
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

    // --------------------------------------------------------
    // Open in Append mode.
    //
    // Policy:
    //   - Do NOT truncate logs on startup.
    //   - Rotation is handled lazily on write (writeToFile()).
    // --------------------------------------------------------
    if (m_logFile.open(QIODevice::Append | QIODevice::Text))
    {
        m_stream.setDevice(&m_logFile);

        m_stream << "\n# ============================================================\n";
        m_stream << "# OCRtoODT Log session start: "
                 << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
                 << "\n";
        m_stream << "# ============================================================\n";
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
    // --------------------------------------------------------
    // Log rotation (size-based)
    // If file exceeds configured limit → rotate
    // --------------------------------------------------------
    if (m_logFile.isOpen())
    {
        if (m_logFile.size() > m_maxLogSizeBytes)
        {
            m_logFile.close();

            const QString baseName = m_logFile.fileName();

            QFile::remove(baseName + ".3");
            QFile::rename(baseName + ".2", baseName + ".3");
            QFile::rename(baseName + ".1", baseName + ".2");
            QFile::rename(baseName,       baseName + ".1");

            if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                m_stream.setDevice(&m_logFile);
            }
            else
            {
                m_consoleEnabled = true;
                writeToConsole("[ERROR] Log rotation failed: cannot reopen log file.");
            }

        }
    }

    if (!m_stream.device())
        return;

    // --------------------------------------------------------
    // Rotation check (approx bytes for UTF-8 line + newline)
    // --------------------------------------------------------
    const qint64 incomingBytes = msg.toUtf8().size() + 1;
    rotateIfNeeded_unlocked(incomingBytes);

    // After rotation we may have a closed stream if reopen failed
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

// ------------------------------------------------------------
// Rotation: check + rotate (caller holds m_mutex)
//
// Policy:
//   - Keep up to 3 backups:
//       log.1 (newest), log.2, log.3 (oldest)
//   - Rotation triggers when текущий файл + incomingBytes > MAX_LOG_SIZE
// ------------------------------------------------------------
void LogRouter::rotateIfNeeded_unlocked(qint64 incomingBytes)
{
    if (!m_fileEnabled)
        return;

    if (!m_logFile.isOpen())
        return;

    const qint64 currentSize = m_logFile.size();
    if (currentSize < 0)
        return;

    if ((currentSize + incomingBytes) <= m_maxLogSizeBytes)
        return;

    rotateLogs_unlocked();
}

void LogRouter::rotateLogs_unlocked()
{
    if (!m_logFile.isOpen())
        return;

    const QString basePath = m_logFile.fileName();
    if (basePath.trimmed().isEmpty())
        return;

    // Ensure everything is flushed before rotating
    m_stream.flush();
    m_logFile.flush();
    m_logFile.close();

    const QString p1 = basePath + ".1";
    const QString p2 = basePath + ".2";
    const QString p3 = basePath + ".3";

    // Delete oldest
    if (QFile::exists(p3))
        QFile::remove(p3);

    // Shift: .2 -> .3, .1 -> .2, base -> .1
    if (QFile::exists(p2))
        QFile::rename(p2, p3);

    if (QFile::exists(p1))
        QFile::rename(p1, p2);

    if (QFile::exists(basePath))
        QFile::rename(basePath, p1);

    // Re-open fresh base file for continued logging
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        m_stream.setDevice(&m_logFile);

        m_stream << "# ============================================================\n";
        m_stream << "# OCRtoODT Log rotated at: "
                 << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")
                 << "\n";
        m_stream << "# Previous file -> " << QFileInfo(p1).fileName() << "\n";
        m_stream << "# ============================================================\n";
        m_stream.flush();
    }
}

// ------------------------------------------------------------
// Set maximum log file size (MB)
// ------------------------------------------------------------
void LogRouter::setMaxLogSizeMB(int megabytes)
{
    QMutexLocker lock(&m_mutex);

    if (megabytes < 1)
        megabytes = 1;

    if (megabytes > 100)
        megabytes = 100;

    m_maxLogSizeBytes = static_cast<qint64>(megabytes) * 1024 * 1024;
}

