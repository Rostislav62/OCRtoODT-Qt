// ============================================================
//  OCRtoODT — Provider Base Class
//  File: core/providers/baseprovider.h
//
//  Responsibility:
//      Common lightweight base class for all provider modules.
//
//      Features:
//          • Stores provider name
//          • Unified logging helpers:
//                logInfo(msg)
//                logWarning(msg)
//                logError(msg)
//                logPerf(msg)
//          • Automatically prepends "[ProviderName]" prefix
//
//      NOTE:
//          Providers no longer emit logMessage signals.
//          All logging goes through LogRouter.
// ============================================================

#ifndef BASEPROVIDER_H
#define BASEPROVIDER_H

#include <QObject>
#include <QString>

namespace Core {

class BaseProvider : public QObject
{
    Q_OBJECT

public:
    explicit BaseProvider(const QString &providerName,
                          QObject *parent = nullptr);

    QString providerName() const { return m_name; }

protected:
    // Logging helpers (prepend provider prefix automatically)
    void logInfo(const QString &msg) const;
    void logWarning(const QString &msg) const;
    void logError(const QString &msg) const;
    void logPerf(const QString &msg) const;

    QString prefixed(const QString &msg) const;

private:
    QString m_name;
};

} // namespace Core

#endif // BASEPROVIDER_H
