// ============================================================
//  OCRtoODT — OCR Profile Storage
//  File: src/core/ocr/OcrProfileStorage.h
//
//  Responsibility:
//      - Persistent storage of OCR language profiles
//      - JSON file located in AppConfigLocation
//
//  JSON Format:
//
//  {
//      "version": 1,
//      "profiles": {
//          "default": {
//              "languages": ["eng", "rus"]
//          },
//          "english_only": {
//              "languages": ["eng"]
//          }
//      }
//  }
//
//  DESIGN RULES:
//      - No UI logic here
//      - No ConfigManager usage
//      - Storage is self-contained and deterministic
// ============================================================

#pragma once

#include <QString>
#include <QStringList>
#include <QMap>

class OcrProfileStorage
{
public:
    OcrProfileStorage();

    // --------------------------------------------------------
    // Load / Save
    // --------------------------------------------------------
    bool load();
    bool save();

    // --------------------------------------------------------
    // Profiles enumeration
    // --------------------------------------------------------
    QStringList profileNames() const;
    bool profileExists(const QString& name) const;

    bool createProfile(const QString& name);
    bool deleteProfile(const QString& name);

    // ------------------------------------------------------------
    // Profile rename (atomic JSON update)
    // ------------------------------------------------------------
    // --------------------------------------------------------
    bool renameProfile(const QString& oldName, const QString& newName);

    // --------------------------------------------------------
    // Languages per profile
    // --------------------------------------------------------
    QStringList languages(const QString& profile) const;
    void setLanguages(const QString& profile,
                      const QStringList& langs);

private:
    QString storagePath() const;
    void ensureDefaultProfile();
    static QString normalizeProfileName(const QString& name);
    static QStringList normalizeLanguages(const QStringList& langs);

private:
    QMap<QString, QStringList> m_profiles;
};
