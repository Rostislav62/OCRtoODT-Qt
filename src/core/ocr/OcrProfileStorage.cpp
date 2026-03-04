// ============================================================
//  OCRtoODT — OCR Profile Storage
//  File: src/core/ocr/OcrProfileStorage.cpp
// ============================================================

#include "OcrProfileStorage.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ============================================================
// Constructor
// ============================================================

OcrProfileStorage::OcrProfileStorage()
{
    load();
}

// ============================================================
// Path resolution
// ============================================================

QString OcrProfileStorage::storagePath() const
{
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    QDir().mkpath(dir);

    return QDir(dir).filePath("ocr_profiles.json");
}

// ============================================================
// Load
// ============================================================

bool OcrProfileStorage::load()
{
    QFile file(storagePath());

    // --------------------------------------------------------
    // If file does not exist — create minimal storage
    // --------------------------------------------------------
    if (!file.exists())
    {
        m_profiles.clear();
        ensureDefaultProfile();
        return save();
    }

    if (!file.open(QIODevice::ReadOnly))
        return false;

    const QJsonDocument doc =
        QJsonDocument::fromJson(file.readAll());

    if (!doc.isObject())
        return false;

    const QJsonObject root = doc.object();
    const QJsonObject profilesObj =
        root.value("profiles").toObject();

    m_profiles.clear();

    for (auto it = profilesObj.begin(); it != profilesObj.end(); ++it)
    {
        const QString profileName =
            normalizeProfileName(it.key());

        const QJsonObject profileObj =
            it.value().toObject();

        const QJsonArray arr =
            profileObj.value("languages").toArray();

        QStringList langs;
        for (const QJsonValue& v : arr)
            langs << v.toString();

        m_profiles.insert(profileName,
                          normalizeLanguages(langs));
    }

    ensureDefaultProfile();
    return true;
}

// ============================================================
// Save
// ============================================================

bool OcrProfileStorage::save()
{
    QJsonObject root;
    root.insert("version", 1);

    QJsonObject profilesObj;

    // Deterministic order
    const QStringList keys = m_profiles.keys();

    for (const QString& name : keys)
    {
        QJsonObject profileObj;
        QJsonArray arr;

        for (const QString& lang : m_profiles.value(name))
            arr.append(lang);

        profileObj.insert("languages", arr);
        profilesObj.insert(name, profileObj);
    }

    root.insert("profiles", profilesObj);

    QFile file(storagePath());
    if (!file.open(QIODevice::WriteOnly))
        return false;

    file.write(
        QJsonDocument(root)
            .toJson(QJsonDocument::Indented));

    return true;
}

// ============================================================
// Profile integrity
// ============================================================

void OcrProfileStorage::ensureDefaultProfile()
{
    // --------------------------------------------------------
    // Business rule alignment:
    // Default profile MUST have at least one language.
    //
    // Reason:
    //   In our pipeline we forbid empty language set.
    //   This prevents "empty OCR" misconfiguration.
    // --------------------------------------------------------
    if (!m_profiles.contains("default") || m_profiles.value("default").isEmpty())
        m_profiles.insert("default", QStringList() << "eng");
}

QString OcrProfileStorage::normalizeProfileName(const QString& name)
{
    QString out = name.trimmed();

    if (out.isEmpty())
        return {};

    out.replace('/', '_');
    out.replace('\\', '_');
    out.replace(':', '_');
    out.replace('.', '_');

    return out;
}

QStringList OcrProfileStorage::normalizeLanguages(const QStringList& langs)
{
    QStringList out;

    for (const QString& s : langs)
    {
        const QString v = s.trimmed();
        if (!v.isEmpty())
            out << v;
    }

    out.removeDuplicates();
    return out;
}

// ============================================================
// Profiles enumeration
// ============================================================

QStringList OcrProfileStorage::profileNames() const
{
    return m_profiles.keys();
}

bool OcrProfileStorage::profileExists(const QString& name) const
{
    return m_profiles.contains(name);
}

bool OcrProfileStorage::createProfile(const QString& name)
{
    const QString normalized =
        normalizeProfileName(name);

    if (normalized.isEmpty())
        return false;

    if (m_profiles.contains(normalized))
        return false;

    m_profiles.insert(normalized, QStringList() << "eng"); // safe non-empty
    return save();
}

bool OcrProfileStorage::deleteProfile(const QString& name)
{
    const QString key = normalizeProfileName(name);
    if (key == "default")
        return false;

    if (!m_profiles.contains(key))
        return false;

    m_profiles.remove(key);
    return save();

    if (!m_profiles.contains(name))
        return false;

    m_profiles.remove(name);
    return save();
}

// ============================================================
// Languages per profile
// ============================================================

QStringList OcrProfileStorage::languages(const QString& profile) const
{
    return m_profiles.value(profile);
}

void OcrProfileStorage::setLanguages(
    const QString& profile,
    const QStringList& langs)
{
    const QString key = normalizeProfileName(profile);
    if (!m_profiles.contains(key))
        return;

    m_profiles[key] = normalizeLanguages(langs);
    save();
}

bool OcrProfileStorage::renameProfile(const QString& oldName, const QString& newName)
{
    const QString oldKey = normalizeProfileName(oldName);
    const QString newKey = normalizeProfileName(newName);

    if (oldKey.isEmpty() || newKey.isEmpty())
        return false;

    // --------------------------------------------------------
    // "default" must always exist and must not be renamed
    // --------------------------------------------------------
    if (oldKey == "default")
        return false;

    // --------------------------------------------------------
    // No-op rename is allowed
    // --------------------------------------------------------
    if (oldKey == newKey)
        return true;

    // Defensive: default must exist even if file was edited manually
    ensureDefaultProfile();

    if (!m_profiles.contains(oldKey))
        return false;

    if (m_profiles.contains(newKey))
        return false;

    // --------------------------------------------------------
    // Atomic key move (in-memory) + single save()
    // --------------------------------------------------------
    m_profiles.insert(newKey, m_profiles.value(oldKey));
    m_profiles.remove(oldKey);

    return save();
}
