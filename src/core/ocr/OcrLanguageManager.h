#pragma once

#include <QObject>
#include <QMap>
#include <QStringList>

#include "LanguageMeta.h"
#include "OcrProfileStorage.h"

// ============================================================
//  OCRtoODT — OCR Language Manager
//
//  Responsibilities:
//      - Load metadata
//      - Scan installed languages
//      - Manage profiles via OcrProfileStorage
//      - Store ONLY active_profile in ConfigManager
// ============================================================

class OcrLanguageManager : public QObject
{
    Q_OBJECT

public:
    static OcrLanguageManager& instance();

    // Metadata
    bool loadMetadata();
    LanguageMeta metaFor(const QString& code) const;
    QString displayNameFor(const QString& code) const;

    // Installed languages
    QStringList scanInstalledLanguages() const;
    QStringList installedLanguages() const;
    void invalidateInstalledCache();
    QString resolvedTessdataDir() const;

    // Profiles
    QStringList profileNames() const;
    QString activeProfile() const;
    void setActiveProfile(const QString& name);

    bool createProfile(const QString& name);
    bool deleteProfile(const QString& name);

    // ------------------------------------------------------------
    // Profile rename (keeps JSON consistent and updates active profile)
    // ------------------------------------------------------------
    bool renameProfile(const QString& oldName, const QString& newName);

    // Active languages
    QStringList activeLanguages() const;
    void setActiveLanguages(const QStringList& langs);

    // --------------------------------------------------------
    // Languages for arbitrary profile (used by Settings UI)
    // --------------------------------------------------------
    QStringList languagesForProfile(const QString& profile) const;
    bool setLanguagesForProfile(const QString& profile,
                                const QStringList& langs);

    // Create new profile with explicit language set (no implicit reads)
    bool createProfile(const QString& name,
                       const QStringList& langs);


    QString buildTesseractLanguageString() const;
    void saveActiveProfile();


signals:
    void languagesChanged();

private:
    explicit OcrLanguageManager(QObject *parent = nullptr);

    QMap<QString, LanguageMeta> m_metadata;
    mutable QStringList m_cachedInstalled;

    OcrProfileStorage m_storage;
};
