// ============================================================
//  OCRtoODT — Performance Profiler
//  File: core/performanceprofiler.cpp
// ============================================================

#include "core/PerformanceProfiler.h"

#include <QDir>
#include <QDateTime>

// ------------------------------------------------------------
// Singleton
// ------------------------------------------------------------
PerformanceProfiler &PerformanceProfiler::instance()
{
    static PerformanceProfiler inst;
    return inst;
}

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
PerformanceProfiler::PerformanceProfiler(QObject *parent)
    : QObject(parent)
{
}

// ------------------------------------------------------------
// ScopedStep implementation
// ------------------------------------------------------------
PerformanceProfiler::ScopedStep::ScopedStep(const QString &name,
                                            int items,
                                            PerformanceProfiler *owner)
    : m_name(name)
    , m_items(items)
    , m_owner(owner)
{
    m_timer.start();
}

PerformanceProfiler::ScopedStep::~ScopedStep()
{
    if (!m_owner)
        return;

    const qint64 ms = m_timer.elapsed();

    QString line = QString("[PERF] %1: %2 ms")
                       .arg(m_name)
                       .arg(ms);

    if (m_items > 0)
    {
        double perItem = (m_items > 0)
        ? double(ms) / double(m_items)
        : 0.0;
        line += QString(" (items=%1, per item=~%2 ms)")
                    .arg(m_items)
                    .arg(perItem, 0, 'f', 2);
    }

    m_owner->writeEntry(line);
    emit m_owner->profileMessage(line);
}

// ------------------------------------------------------------
// Create scoped guard
// ------------------------------------------------------------
std::unique_ptr<PerformanceProfiler::ScopedStep>
PerformanceProfiler::scope(const QString &name, int items)
{
    return std::make_unique<ScopedStep>(name, items, this);
}

// ------------------------------------------------------------
// Ensure log file is open
// ------------------------------------------------------------
void PerformanceProfiler::ensureLogFile()
{
    if (m_logFile.isOpen())
        return;

    QDir dir;
    const QString logDir = QStringLiteral("logs");
    if (!dir.exists(logDir))
        dir.mkpath(logDir);

    const QString ts =
        QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");

    const QString path =
        dir.filePath(QString("logs/perf_%1.txt").arg(ts));

    m_logFile.setFileName(path);
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        m_stream.setDevice(&m_logFile);
        m_stream << "# OCRtoODT Performance Log " << ts << "\n";
        m_stream.flush();
    }
}

// ------------------------------------------------------------
// Write a single entry
// ------------------------------------------------------------
void PerformanceProfiler::writeEntry(const QString &line)
{
    ensureLogFile();

    if (m_stream.device())
    {
        m_stream << line << "\n";
        m_stream.flush();
    }
}
