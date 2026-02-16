// ============================================================
//  OCRtoODT — Config Manager (Hierarchical, Comment-Preserving)
//  File: core/configmanager.h
//  NEW
// ============================================================

#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QString>
#include <QStringList>
#include <QVariant>

class ConfigManager
{
public:
    // --------------------------------------------------------
    // Singleton
    // --------------------------------------------------------
    static ConfigManager& instance();

    // --------------------------------------------------------
    // Load / Reload / Save
    // --------------------------------------------------------

    // Load config.yaml into internal line buffer.
    // Performs automatic migration after load.
    bool load(const QString &path);

    // Reload from the last loaded path.
    bool reload();

    // Save current buffer back to the same file.
    bool save();

    // --------------------------------------------------------
    // Get / Set values
    // --------------------------------------------------------

    // Read scalar value by dot-separated path.
    QVariant get(const QString &path,
                 const QVariant &defaultValue = QVariant()) const;

    // Update scalar value by path.
    // If key does not exist, it will be created conservatively.
    bool set(const QString &path, const QVariant &value);

    // --------------------------------------------------------
    // Debug helpers
    // --------------------------------------------------------
    void dump() const;

private:
    ConfigManager() = default;

private:
    // --------------------------------------------------------
    // Internal storage
    // --------------------------------------------------------

    // Raw YAML lines (preserved verbatim)
    QStringList m_lines;

    // Last loaded config file path
    QString m_filePath;

private:
    // --------------------------------------------------------
    // Internal helpers
    // --------------------------------------------------------

    // Convert QVariant into YAML-safe scalar string
    QString buildYamlValue(const QVariant &value) const;

    // Ensure key exists, inserting default value if missing
    bool ensureKeyExists(const QString &path,
                         const QVariant &defaultValue);

    // Perform automatic key migration after load
    void migrate();
};

#endif // CONFIGMANAGER_H
