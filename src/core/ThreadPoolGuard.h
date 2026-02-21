// ============================================================
//  OCRtoODT — ThreadPoolGuard
//  File: src/core/ThreadPoolGuard.h
//
//  Responsibility:
//      Centralized control over QThreadPool::globalInstance().
//
//      Guarantees:
//          • Deterministic upper bound of parallel OCR jobs
//          • Protection from runaway parallelism
//          • Stable runtime behavior across machines
//
//  Policy:
//      • Applied once during application startup
//      • Does NOT read config.yaml directly
//      • Receives effective runtime values from main()
// ============================================================

#pragma once

#include <QString>

class ThreadPoolGuard
{
public:
    // Apply global thread pool limits.
    //
    // parallelEnabled  — effective parallel flag
    // numProcesses     — "auto" or numeric string
    // cpuLogical       — detected logical CPU threads
    static void apply(bool parallelEnabled,
                      const QString& numProcesses,
                      int cpuLogical);
};
