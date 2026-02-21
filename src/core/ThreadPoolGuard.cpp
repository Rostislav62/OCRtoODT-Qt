// ============================================================
//  OCRtoODT — ThreadPoolGuard
//  File: src/core/ThreadPoolGuard.cpp
// ============================================================

#include "core/ThreadPoolGuard.h"

#include <QThreadPool>
#include <QtGlobal>

#include "core/LogRouter.h"

void ThreadPoolGuard::apply(bool parallelEnabled,
                            const QString& numProcesses,
                            int cpuLogical)
{
    QThreadPool* pool = QThreadPool::globalInstance();

    const int previous = pool->maxThreadCount();

    int newLimit = 1;

    if (!parallelEnabled)
    {
        newLimit = 1;
    }
    else
    {
        if (numProcesses == "auto")
        {
            newLimit = qMax(1, cpuLogical);
        }
        else
        {
            bool ok = false;
            int parsed = numProcesses.toInt(&ok);

            if (ok && parsed > 0)
                newLimit = parsed;
            else
                newLimit = qMax(1, cpuLogical);
        }
    }

    pool->setMaxThreadCount(newLimit);

    LogRouter::instance().info(
        QString("[ThreadPoolGuard] globalInstance maxThreadCount: %1 → %2")
            .arg(previous)
            .arg(newLimit));
}
