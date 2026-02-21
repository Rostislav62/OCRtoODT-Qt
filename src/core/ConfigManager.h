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
#include <QRecursiveMutex>
#include <QSet>


class ConfigManager
{
public:
    // --------------------------------------------------------
    // Singleton
    // --------------------------------------------------------
    static ConfigManager& instance();

    // --------------------------------------------------------
    // Mode control (Development / Production)
    // --------------------------------------------------------
    enum class Mode
    {
        Development,
        Production
    };

    void setMode(Mode mode);
    Mode mode() const;

    // --------------------------------------------------------
    // Load / Reload / Save
    // --------------------------------------------------------
    bool load(const QString &path);
    bool reload();
    bool save();

    // --------------------------------------------------------
    // Import / Export
    // --------------------------------------------------------
    // Export current in-memory config (m_lines) into an arbitrary file.
    // Atomic write (QSaveFile). Does NOT change m_filePath.
    bool exportToFile(const QString &path) const;

    // Import config from an arbitrary file into the CURRENT active config path (m_filePath).
    // Policy:
    //   - Read file as text lines (preserve comments)
    //   - Validate schema (v1.0) before applying
    //   - Create backup of current config.yaml
    //   - Replace current config.yaml atomically
    //   - Reload into memory
    bool importFromFile(const QString &path);

    // --------------------------------------------------------
    // Get / Set values
    // --------------------------------------------------------
    QVariant get(const QString &path,
                 const QVariant &defaultValue = QVariant()) const;

    // Update scalar value by path.
    // In Production: if key does not exist, set() FAILS (no auto-insert).
    bool set(const QString &path, const QVariant &value);

    // --------------------------------------------------------
    // Migration journal (audit)
    // --------------------------------------------------------
    QStringList migrationLog() const;
    void clearMigrationLog();

    // --------------------------------------------------------
    // Debug helpers
    // --------------------------------------------------------
    void dump() const;

    // --------------------------------------------------------
    // Validation (Production safety)
    // --------------------------------------------------------

    // Validate that config.yaml matches the fixed schema:
    //  - only known keys
    //  - max nesting depth: 2
    //  - scalar-only values
    //  - enums/ranges checked for critical fields
    bool validateConfigStructure();

    bool validationFailed() const;

    // --------------------------------------------------------
    // Reset entire config.yaml to canonical defaults (schema v1.0)
    //
    // Behavior:
    //   - Overwrites all allowed scalar keys
    //   - Preserves comment structure where possible
    //   - Performs atomic save
    //   - Reloads config from disk
    //
    // Returns true on success.
    // --------------------------------------------------------
    bool resetToDefaults();




private:
    ConfigManager() = default;

    // --------------------------------------------------------
    // Internal storage
    // --------------------------------------------------------
    QStringList m_lines;
    QString m_filePath;

    // --------------------------------------------------------
    // Thread safety
    // --------------------------------------------------------
    mutable QRecursiveMutex m_mutex;

    // --------------------------------------------------------
    // Deterministic behavior mode
    // --------------------------------------------------------
    Mode m_mode = Mode::Production;

    // --------------------------------------------------------
    // Migration journal
    // --------------------------------------------------------
    QStringList m_migrationLog;

    // --------------------------------------------------------
    // Internal helpers
    // --------------------------------------------------------
    QString buildYamlValue(const QVariant &value) const;

    // Ensure key exists, inserting default value if missing
    // In Production: forbidden (returns false).
    bool ensureKeyExists(const QString &path,
                         const QVariant &defaultValue);

    // Perform automatic key migration after load
    // In Production: journal-only (no writes).
    void migrate();

    // Record audit entries (used by migrate/ensureKeyExists/set)
    void recordMigration(const QString &line);

    // Parse YAML (our restricted format) and return all scalar keys found.
    // Example returned key: "ui.theme_mode"
    QStringList listScalarKeys() const;

    // Return the set of allowed scalar keys (KnownKeyRegistry).
    static QSet<QString> allowedScalarKeys();

    // --------------------------------------------------------
    // Default registry (schema v1.0)
    // --------------------------------------------------------
    //
    // Canonical default values for ALL allowed scalar keys.
    // This is the single source of truth for:
    //
    //   - Reset to defaults
    //   - Initial seeding
    //   - Future schema migrations
    //
    // Must stay consistent with allowedScalarKeys().
    //
    static QMap<QString, QVariant> defaultScalarValues();

    #ifndef QT_NO_DEBUG
        static void debugAssertSchemaRegistrySyncOnce();
    #endif


    // Validate a single key/value pair for enums/ranges.
    bool validateValueForKey(const QString &fullKey,
                             const QString &rawValue) const;

    bool m_validationFailed = false;



};

#endif // CONFIGMANAGER_H
