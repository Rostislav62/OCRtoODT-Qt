// ============================================================
//  OCRtoODT — OCR Language Manager
//  File: src/core/ocr/OcrLanguageManager.cpp
//
//  Architecture:
//      - Language metadata from resource JSON
//      - Installed languages from tessdata scan
//      - Profiles stored ONLY in OcrProfileStorage (JSON file)
//      - Config stores ONLY active_profile
// ============================================================

#include "OcrLanguageManager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDir>
#include <QFileInfo>
#include <QCoreApplication>

#include "core/ConfigManager.h"
#include "core/LanguageManager.h"
#include "core/LogRouter.h"

static const char* KEY_ACTIVE_PROFILE = "ocr.active_profile";
static const char* KEY_TESSDATA_DIR   = "ocr.tessdata_dir";

// ============================================================
// Singleton
// ============================================================

OcrLanguageManager& OcrLanguageManager::instance()
{
    static OcrLanguageManager inst;
    return inst;
}

OcrLanguageManager::OcrLanguageManager(QObject *parent)
    : QObject(parent)
{
    // --------------------------------------------------------
    // Ensure profile storage is loaded
    // --------------------------------------------------------
    m_storage.load();

    // --------------------------------------------------------
    // Backward compatibility migration:
    // If legacy key "ocr.languages" exists and default profile
    // is still minimal — migrate it once into "default" profile.
    //
    // We DO NOT delete legacy key here (safe, non-destructive).
    // --------------------------------------------------------
    {
        ConfigManager &cfg = ConfigManager::instance();

        const QString legacy =
            cfg.get("ocr.languages", "").toString().trimmed();

        // legacy format: "rus,eng" or "eng"
        if (!legacy.isEmpty())
        {
            QStringList legacyList =
                legacy.split(',', Qt::SkipEmptyParts);

            for (QString &s : legacyList)
                s = s.trimmed();

            legacyList.removeAll(QString());
            legacyList.removeDuplicates();

            // If default profile is still only "eng" (or empty),
            // and legacy has something meaningful — migrate.
            const QStringList currentDefault =
                m_storage.languages("default");

            const bool defaultIsMinimal =
                (currentDefault.isEmpty() ||
                 (currentDefault.size() == 1 && currentDefault.first() == "eng"));

            if (defaultIsMinimal && !legacyList.isEmpty())
            {
                m_storage.setLanguages("default", legacyList);

                LogRouter::instance().info(
                    QString("[OcrLanguageManager] Migrated legacy ocr.languages -> default profile: %1")
                        .arg(legacyList.join(",")));
            }
        }
    }
}

// ============================================================
// Metadata
// ============================================================

bool OcrLanguageManager::loadMetadata()
{
    QFile file(":/resources/ocr_languages.json");

    if (!file.open(QIODevice::ReadOnly))
    {
        LogRouter::instance().warning("[OcrLanguageManager] Cannot open metadata JSON");
        return false;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);

    if (err.error != QJsonParseError::NoError)
    {
        LogRouter::instance().warning(
            QString("[OcrLanguageManager] JSON parse error: %1").arg(err.errorString()));
        return false;
    }

    m_metadata.clear();

    const QJsonObject root = doc.object();

    for (auto it = root.begin(); it != root.end(); ++it)
    {
        LanguageMeta meta;
        meta.code = it.key();

        const QJsonObject obj = it.value().toObject();
        meta.nameEn  = obj.value("name_en").toString();
        meta.nameRu  = obj.value("name_ru").toString();
        meta.iso639_1 = obj.value("iso639_1").toString();

        m_metadata.insert(meta.code, meta);
    }

    LogRouter::instance().info(
        QString("[OcrLanguageManager] Loaded metadata entries: %1")
            .arg(m_metadata.size()));

    return true;
}

LanguageMeta OcrLanguageManager::metaFor(const QString& code) const
{
    return m_metadata.value(code);
}

QString OcrLanguageManager::displayNameFor(const QString& code) const
{
    const LanguageMeta meta = metaFor(code);

    if (meta.code.isEmpty())
        return code;

    const QString uiLang = LanguageManager::instance()->currentLanguage();
    const bool preferRu = uiLang.startsWith("ru", Qt::CaseInsensitive);

    QString name =
        preferRu
            ? (meta.nameRu.isEmpty() ? meta.nameEn : meta.nameRu)
            : (meta.nameEn.isEmpty() ? meta.nameRu : meta.nameEn);

    if (name.isEmpty())
        return code;

    return QString("%1 (%2)").arg(name, code);
}

// ============================================================
// Tessdata scan
// ============================================================
QString OcrLanguageManager::resolvedTessdataDir() const
{
    ConfigManager& cfg = ConfigManager::instance();

    QString tessDir =
        cfg.get(KEY_TESSDATA_DIR, "tessdata").toString().trimmed();

    QDir dir(tessDir);

    if (dir.isRelative())
    {
        const QString exeDir =
            QCoreApplication::applicationDirPath();

        tessDir = QDir(exeDir).filePath(tessDir);
        dir = QDir(tessDir);
    }

    const QString dirPath = dir.absolutePath();

    if (!dir.exists())
        QDir().mkpath(dirPath);

    LogRouter::instance().info(
        QString("[OcrLanguageManager] Tessdata dir: %1").arg(dirPath));

    return dirPath;
}

QStringList OcrLanguageManager::scanInstalledLanguages() const
{
    const QString tessDir = resolvedTessdataDir();
    QDir dir(tessDir);

    if (!dir.exists())
    {
        LogRouter::instance().warning(
            QString("[OcrLanguageManager] tessdata dir not found: %1")
                .arg(tessDir));
        return {};
    }

    QStringList result;

    const QFileInfoList files =
        dir.entryInfoList(QStringList() << "*.traineddata", QDir::Files);

    for (const QFileInfo& fi : files)
    {
        const QString code = fi.completeBaseName();

        if (code == "osd" || code == "equ")
            continue;

        result << code;
    }

    result.removeDuplicates();
    result.sort(Qt::CaseInsensitive);

    LogRouter::instance().info(
        QString("[OcrLanguageManager] Installed languages: %1")
            .arg(result.join(",")));

    return result;
}

QStringList OcrLanguageManager::installedLanguages() const
{
    if (m_cachedInstalled.isEmpty())
        m_cachedInstalled = scanInstalledLanguages();

    return m_cachedInstalled;
}

void OcrLanguageManager::invalidateInstalledCache()
{
    m_cachedInstalled.clear();
}

// ============================================================
// Profiles (JSON storage based)
// ============================================================

QStringList OcrLanguageManager::profileNames() const
{
    return m_storage.profileNames();
}

QString OcrLanguageManager::activeProfile() const
{
    const QString current =
        ConfigManager::instance()
            .get(KEY_ACTIVE_PROFILE, "default")
            .toString()
            .trimmed();

    if (current.isEmpty())
        return "default";

    if (!m_storage.profileExists(current))
        return "default";

    return current;
}

void OcrLanguageManager::setActiveProfile(const QString& name)
{
    if (!m_storage.profileExists(name))
        return;

    ConfigManager::instance().set(KEY_ACTIVE_PROFILE, name);
    ConfigManager::instance().save();

    emit languagesChanged();
}

// ============================================================
// Create new profile
// ============================================================

bool OcrLanguageManager::createProfile(
    const QString& name,
    const QStringList& langs)
{
    const QString trimmed = name.trimmed();

    if (trimmed.isEmpty())
        return false;

    // --------------------------------------------------------
    // Business rule: empty profile is forbidden
    // --------------------------------------------------------
    if (langs.isEmpty())
    {
        LogRouter::instance().warning(
            "[OcrLanguageManager] createProfile rejected: empty language set");
        return false;
    }

    const bool ok = m_storage.createProfile(trimmed);
    if (!ok)
        return false;

    m_storage.setLanguages(trimmed, langs);

    emit languagesChanged();
    return true;
}

bool OcrLanguageManager::createProfile(const QString& name)
{
    // Backward-compatible behavior: create from current active languages
    return createProfile(name, activeLanguages());
}

bool OcrLanguageManager::deleteProfile(const QString& name)
{
    const bool ok = m_storage.deleteProfile(name);
    if (ok)
        emit languagesChanged();
    return ok;
}

bool OcrLanguageManager::renameProfile(const QString& oldName, const QString& newName)
{
    const QString oldKey = oldName.trimmed();
    const QString newKey = newName.trimmed();

    if (oldKey.isEmpty() || newKey.isEmpty())
        return false;

    // "default" is immutable
    if (oldKey == "default")
        return false;

    // --------------------------------------------------------
    // Rename in storage first (atomic JSON update)
    // --------------------------------------------------------
    const bool ok = m_storage.renameProfile(oldKey, newKey);
    if (!ok)
        return false;

    // --------------------------------------------------------
    // If renamed profile was active -> update config key as well
    // --------------------------------------------------------
    if (activeProfile() == oldKey)
    {
        setActiveProfile(newKey);
        saveActiveProfile();
    }

    LogRouter::instance().info(
        QString("[OcrLanguageManager] Profile renamed: '%1' -> '%2'")
            .arg(oldKey, newKey));

    emit languagesChanged();
    return true;
}

// ============================================================
// Active profile languages
// ============================================================

QStringList OcrLanguageManager::activeLanguages() const
{
    return m_storage.languages(activeProfile());
}

void OcrLanguageManager::setActiveLanguages(const QStringList& langs)
{
    // --------------------------------------------------------
    // Validation: language set must not be empty
    // Business rule: profile cannot contain zero languages
    // --------------------------------------------------------
    if (langs.isEmpty())
    {
        LogRouter::instance().warning(
            "[OcrLanguageManager] setActiveLanguages rejected: empty language set");
        return;
    }

    m_storage.setLanguages(activeProfile(), langs);

    emit languagesChanged();
}

QString OcrLanguageManager::buildTesseractLanguageString() const
{
    const QStringList installed = installedLanguages();
    const QStringList active = activeLanguages();

    QStringList final;

    for (const QString& lang : active)
    {
        if (installed.contains(lang))
            final << lang;
        else
            LogRouter::instance().warning(
                QString("[OcrLanguageManager] Language not installed: %1")
                    .arg(lang));
    }

    const QString result = final.join('+');

    LogRouter::instance().info(
        QString("[OcrLanguageManager] Final Tesseract string: %1")
            .arg(result));

    return result;
}

void OcrLanguageManager::saveActiveProfile()
{
    ConfigManager::instance().save();
}


// ============================================================
// Languages per profile (explicit accessors for Settings UI)
// ============================================================

QStringList OcrLanguageManager::languagesForProfile(const QString& profile) const
{
    if (profile.trimmed().isEmpty())
        return {};

    if (!m_storage.profileExists(profile))
        return {};

    return m_storage.languages(profile);
}

bool OcrLanguageManager::setLanguagesForProfile(
    const QString& profile,
    const QStringList& langs)
{
    const QString trimmed = profile.trimmed();

    if (trimmed.isEmpty())
        return false;

    if (!m_storage.profileExists(trimmed))
        return false;

    // --------------------------------------------------------
    // Business rule: profile cannot contain zero languages
    // --------------------------------------------------------
    if (langs.isEmpty())
    {
        LogRouter::instance().warning(
            "[OcrLanguageManager] setLanguagesForProfile rejected: empty language set");
        return false;
    }

    m_storage.setLanguages(trimmed, langs);
    emit languagesChanged();
    return true;
}
