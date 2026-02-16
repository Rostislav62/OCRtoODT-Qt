// ============================================================
//  OCRtoODT — Language Manager
//  File: src/core/LanguageManager.h
//
//  Responsibility:
//      - Centralized application language management
//      - Load and install QTranslator at runtime
//      - Auto-detect language (system locale → fallback)
//      - Live language switching without application restart
//
//  Design rules:
//      - Singleton (like ThemeManager, ConfigManager)
//      - Owns the ONLY global QTranslator
//      - Does NOT depend on UI widgets
// ============================================================

#ifndef LANGUAGEMANAGER_H
#define LANGUAGEMANAGER_H

#include <QObject>
#include <QTranslator>
#include <QString>
#include <QStringList>

class LanguageManager : public QObject
{
    Q_OBJECT

public:
    static LanguageManager *instance();

    // Initialize language at application startup
    void initialize();

    // Apply language explicitly (e.g. from settings)
    bool setLanguage(const QString &langCode);

    // Current active language code (en / ru / ro / ...)
    QString currentLanguage() const;

    // List of supported languages (compiled .qm present)
    QStringList availableLanguages() const;

signals:
    // Emitted AFTER translator is installed
    void languageChanged(const QString &langCode);

private:
    explicit LanguageManager(QObject *parent = nullptr);

    // Auto-detect based on system locale + fallback
    QString detectInitialLanguage() const;

    // Resolve .qm path on disk (runtime search)
    QString resolveQmPath(const QString &langCode) const;

private:
    QTranslator m_translator;
    QString     m_currentLanguage;
};

#endif // LANGUAGEMANAGER_H
