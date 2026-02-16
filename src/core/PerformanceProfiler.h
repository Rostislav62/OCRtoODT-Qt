// ============================================================
//  OCRtoODT — Performance Profiler
//  File: core/performanceprofiler.h
//
//  Responsibility:
//      Lightweight, global profiler for OCR pipeline steps.
//      Allows each stage (normalize, preprocess, OCR, layout,
//      document build) to report timing information.
//
//      Usage:
//          auto guard = PerformanceProfiler::instance()
//                          .scope("Normalize step 1.1", pageCount);
//          ... do work ...
//          // on guard destruction, time is recorded.
//
//      Output:
//          - log file:  logs/perf_YYYYMMDD_HHMMSS.txt
//          - optional signal for UI log (MainWindow)
// ============================================================

#ifndef PERFORMANCEPROFILER_H
#define PERFORMANCEPROFILER_H

#include <QObject>
#include <QElapsedTimer>
#include <QFile>
#include <QTextStream>
#include <memory>

class PerformanceProfiler : public QObject
{
    Q_OBJECT

public:
    // --------------------------------------------------------
    // Singleton access
    // --------------------------------------------------------
    static PerformanceProfiler &instance();

    // --------------------------------------------------------
    // RAII helper for measuring one named step.
    //
    // Example:
    //      auto guard = profiler.scope("Normalize", pages.size());
    // --------------------------------------------------------
    class ScopedStep
    {
    public:
        ScopedStep(const QString &name,
                   int items,
                   PerformanceProfiler *owner);
        ~ScopedStep();

        ScopedStep(const ScopedStep &) = delete;
        ScopedStep &operator=(const ScopedStep &) = delete;

    private:
        QString             m_name;
        int                 m_items = 0;
        PerformanceProfiler *m_owner = nullptr;
        QElapsedTimer       m_timer;
    };

    // Create scoped guard
    std::unique_ptr<ScopedStep> scope(const QString &name, int items);

signals:
    // For UI log (MainWindow can connect to this).
    void profileMessage(const QString &msg);

private:
    explicit PerformanceProfiler(QObject *parent = nullptr);
    Q_DISABLE_COPY(PerformanceProfiler)

    void ensureLogFile();
    void writeEntry(const QString &line);

private:
    QFile       m_logFile;
    QTextStream m_stream;
};

#endif // PERFORMANCEPROFILER_H
