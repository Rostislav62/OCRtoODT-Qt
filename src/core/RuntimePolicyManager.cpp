// ============================================================
//  OCRtoODT — RuntimePolicyManager
//  File: src/core/RuntimePolicyManager.cpp
//
//  Stage 4.4 (B) — Dynamic Memory Pressure
//
//  Responsibilities:
//      • Read user intent from ConfigManager
//      • Dynamically evaluate free RAM before each RUN
//      • Compute effective runtime state
//      • Publish effective values into ConfigManager (in-memory)
//      • Apply ThreadPoolGuard
//      • Defer reapply if OCR is active
//
// ============================================================

#include "core/RuntimePolicyManager.h"

#include "core/ConfigManager.h"
#include "core/ThreadPoolGuard.h"
#include "core/LogRouter.h"
#include "systeminfo/systeminfo.h"

#include <atomic>

// ------------------------------------------------------------
// Cached hardware snapshot
// ------------------------------------------------------------

static int g_cpuLogical = 1;

// Deferred reapply flag (if settings changed mid-OCR)
static std::atomic_bool g_pendingReapply{false};

// ------------------------------------------------------------
// User intent (persistent config values)
// ------------------------------------------------------------

struct UserState
{
    bool parallelEnabled = true;
    QString numProcesses = "auto";
    QString dataMode     = "auto";
};

// ------------------------------------------------------------
// Effective runtime state (session-level)
// ------------------------------------------------------------

struct RuntimeState
{
    bool parallelEnabled = true;
    QString numProcesses = "auto";
    QString dataMode     = "ram_only";
};

// ------------------------------------------------------------
// Read user state from config
// ------------------------------------------------------------

static UserState readUserState()
{
    ConfigManager &cfg = ConfigManager::instance();

    UserState u;
    u.parallelEnabled = cfg.get("general.parallel_enabled", true).toBool();
    u.numProcesses    = cfg.get("general.num_processes", "auto").toString().trimmed();
    u.dataMode        = cfg.get("general.mode", "auto").toString().trimmed();

    return u;
}

// ------------------------------------------------------------
// Normalize worker count
// ------------------------------------------------------------

static QString normalizeWorkers(const QString &raw, bool parallelEnabled)
{
    if (!parallelEnabled)
        return "1";

    if (raw.isEmpty() || raw == "auto")
        return "auto";

    bool ok = false;
    int n = raw.toInt(&ok);
    if (!ok || n < 1)
        return "auto";

    return QString::number(n);
}

static int effectiveWorkerCount(const QString &num, bool parallelEnabled)
{
    if (!parallelEnabled)
        return 1;

    if (num == "auto")
        return g_cpuLogical;

    bool ok = false;
    int n = num.toInt(&ok);
    if (!ok || n < 1)
        return g_cpuLogical;

    return qMin(n, g_cpuLogical);
}

// ------------------------------------------------------------
// Memory model (conservative)
// ------------------------------------------------------------

static QString decideAutoMode(int workers)
{
    const long long freeMB = si_free_ram_mb();

    // Emergency hard guard
    if (freeMB <= 0 || freeMB < 4096)
        return "disk_only";

    const long long perPageMB   = 32;
    const double    safety      = 1.5;
    const long long reserveMB   = 1024;

    const long long required =
        static_cast<long long>(workers * perPageMB * safety + reserveMB);

    return (freeMB >= required) ? "ram_only" : "disk_only";
}

// ------------------------------------------------------------
// Compute effective runtime state
// ------------------------------------------------------------

static RuntimeState computeRuntimeState(const UserState &u)
{
    RuntimeState r;

    r.parallelEnabled = u.parallelEnabled;

    if (g_cpuLogical <= 1)
        r.parallelEnabled = false;

    r.numProcesses = normalizeWorkers(u.numProcesses, r.parallelEnabled);

    const int workers = effectiveWorkerCount(r.numProcesses, r.parallelEnabled);

    QString userMode = u.dataMode.isEmpty() ? "auto" : u.dataMode;

    if (userMode == "ram_only" || userMode == "disk_only")
    {
        r.dataMode = userMode;
    }
    else
    {
        r.dataMode = decideAutoMode(workers);
    }

    LogRouter::instance().info(
        QString("[RuntimePolicy] workers=%1 mode=%2 freeRAM=%3MB")
            .arg(workers)
            .arg(r.dataMode)
            .arg(si_free_ram_mb()));

    return r;
}

// ------------------------------------------------------------
// Publish effective state back into config (in-memory)
// ------------------------------------------------------------

static void publish(const RuntimeState &r)
{
    ConfigManager &cfg = ConfigManager::instance();

    cfg.set("general.parallel_enabled", r.parallelEnabled);
    cfg.set("general.num_processes", r.numProcesses);
    cfg.set("general.mode", r.dataMode);
}

// ============================================================
// Public API
// ============================================================

void RuntimePolicyManager::initialize(int cpuLogical)
{
    g_cpuLogical = (cpuLogical < 1) ? 1 : cpuLogical;

    reapply();
}

void RuntimePolicyManager::reapply()
{
    const UserState user = readUserState();
    const RuntimeState runtime = computeRuntimeState(user);

    publish(runtime);

    ThreadPoolGuard::apply(
        runtime.parallelEnabled,
        runtime.numProcesses,
        g_cpuLogical);

    LogRouter::instance().info("[RuntimePolicy] reapplied.");
}

void RuntimePolicyManager::requestReapply(bool ocrIsRunning)
{
    if (ocrIsRunning)
    {
        g_pendingReapply.store(true);
        LogRouter::instance().info("[RuntimePolicy] deferred (OCR running).");
        return;
    }

    reapply();
}

void RuntimePolicyManager::onPipelineBecameIdle()
{
    if (!g_pendingReapply.exchange(false))
        return;

    LogRouter::instance().info("[RuntimePolicy] applying deferred policy.");
    reapply();
}
