// ============================================================
//  OCRtoODT — Provider Base Class
//  File: core/providers/baseprovider.cpp
// ============================================================

#include "core/providers/baseProvider.h"
#include "core/LogRouter.h"

namespace Core {

BaseProvider::BaseProvider(const QString &providerName,
                           QObject *parent)
    : QObject(parent)
    , m_name(providerName)
{
}

// ------------------------------------------------------------
// Prefix helper
// ------------------------------------------------------------
QString BaseProvider::prefixed(const QString &msg) const
{
    return QString("[%1] %2").arg(m_name, msg);
}

// ------------------------------------------------------------
// Logging wrappers
// ------------------------------------------------------------
void BaseProvider::logInfo(const QString &msg) const
{
    LogRouter::instance().info(prefixed(msg));
}

void BaseProvider::logWarning(const QString &msg) const
{
    LogRouter::instance().warning(prefixed(msg));
}

void BaseProvider::logError(const QString &msg) const
{
    LogRouter::instance().error(prefixed(msg));
}

void BaseProvider::logPerf(const QString &msg) const
{
    LogRouter::instance().perf(prefixed(msg));
}

} // namespace Core
