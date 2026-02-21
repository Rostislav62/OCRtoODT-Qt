// ============================================================
//  OCRtoODT — Config Manager (Hierarchical, Comment-Preserving)
//  File: core/configmanager.cpp
// ============================================================

#include "ConfigManager.h"

#include "core/LogRouter.h" // CHANGED: unified logging via LogRouter

#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QSaveFile>
#include <QSet>
#include <QRegularExpression>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QStringList>
#include <QMap>

// ============================================================
// Singleton
// ============================================================
ConfigManager &ConfigManager::instance()
{
    static ConfigManager inst;

#ifndef QT_NO_DEBUG
    debugAssertSchemaRegistrySyncOnce();
#endif

    return inst;
}


// ============================================================
// Operating mode (Production / Development)
// ============================================================

void ConfigManager::setMode(ConfigManager::Mode mode)
{
    QMutexLocker lock(&m_mutex);
    m_mode = mode;
}

ConfigManager::Mode ConfigManager::mode() const
{
    QMutexLocker lock(&m_mutex);
    return m_mode;
}

// ============================================================
// Migration journal
// ============================================================
QStringList ConfigManager::migrationLog() const
{
    QMutexLocker lock(&m_mutex);
    return m_migrationLog;
}

void ConfigManager::clearMigrationLog()
{
    QMutexLocker lock(&m_mutex);
    m_migrationLog.clear();
}

void ConfigManager::recordMigration(const QString &line)
{
    // NOTE: caller holds lock (or recursion-safe)
    m_migrationLog.append(line);
}




// ============================================================
// Load YAML file into raw text buffer
// ============================================================
bool ConfigManager::load(const QString &path)
{
    QMutexLocker lock(&m_mutex);

    m_filePath = path;
    m_migrationLog.clear();

    LogRouter::instance().info(
        QString("[ConfigManager] load() path = %1").arg(m_filePath));

    m_lines.clear();

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        // CHANGED: qWarning() -> LogRouter
        LogRouter::instance().error(
            QString("[ConfigManager] Cannot open config file: %1").arg(path));
        return false;
    }

    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);

    while (!ts.atEnd())
        m_lines.append(ts.readLine());

    // After load — perform automatic key migration
    migrate();

    // --------------------------------------------------------
    // Validate structure (Production safety)
    // --------------------------------------------------------
    validateConfigStructure();

    return true;
}

// ============================================================
// Reload config.yaml from the last loaded path
// ============================================================
bool ConfigManager::reload()
{
    QMutexLocker lock(&m_mutex);

    if (m_filePath.trimmed().isEmpty())
    {
        LogRouter::instance().warning(
            "[ConfigManager] reload() called but no file was loaded yet.");
        return false;
    }

    const QString path = m_filePath; // copy under lock
    lock.unlock();                   // avoid holding lock across nested load
    return load(path);
}



// ============================================================
// Convert QVariant to YAML-safe scalar
// ============================================================
QString ConfigManager::buildYamlValue(const QVariant &value) const
{
    if (value.metaType().id() == QMetaType::Bool)
        return value.toBool() ? "true" : "false";

    // Strings and numbers are written as-is
    return value.toString();
}


// ============================================================
// KnownKeyRegistry (allowed scalar keys)
// ============================================================
QSet<QString> ConfigManager::allowedScalarKeys()
{
    return QSet<QString>{

        // ----------------------------------------------------
        // Config
        // ----------------------------------------------------
        "config.version",

            // ----------------------------------------------------
            // Logging
            // ----------------------------------------------------
            "logging.enabled",
            "logging.level",
            "logging.file_output",
            "logging.gui_output",
            "logging.console_output",
            "logging.file_path",
            "logging.max_file_size_mb",

            // ----------------------------------------------------
            // UI
            // ----------------------------------------------------
            "ui.theme_mode",
            "ui.custom_qss",
            "ui.app_font_family",
            "ui.app_font_size",
            "ui.font_size",              // NEW (schema-level key)
            "ui.log_font_size",
            "ui.toolbar_style",
            "ui.toolbar_icon_size",      // NEW
            "ui.thumbnail_size",
            "ui.language",               // NEW
            "ui.expert_mode",
            "ui.notify_on_finish",
            "ui.play_sound_on_finish",
            "ui.sound_volume",
            "ui.sound_path",
            "ui.show_preprocess_tab",
            "ui.show_logging_tab",

            // ----------------------------------------------------
            // General
            // ----------------------------------------------------
            "general.parallel_enabled",
            "general.num_processes",
            "general.mode",
            "general.debug_mode",
            "general.input_dir",
            "general.preprocess_path",
            "general.output_file",       // NEW
            "general.ocr_path",          // NEW

            // ----------------------------------------------------
            // Preprocess
            // ----------------------------------------------------
            "preprocess.profile",

            // ----------------------------------------------------
            // Recognition
            // ----------------------------------------------------
            "recognition.language",
            "recognition.psm",

            // ----------------------------------------------------
            // ODT
            // ----------------------------------------------------
            "odt.font_family",
            "odt.font_size",
            "odt.justify",

            "odt.paper_size",                    // NEW
            "odt.font_name",                     // NEW
            "odt.font_size_pt",                  // NEW
            "odt.text_align",                    // NEW
            "odt.first_line_indent_mm",          // NEW
            "odt.paragraph_spacing_after_pt",    // NEW
            "odt.line_height_percent",           // NEW
            "odt.margin_left_mm",                // NEW
            "odt.margin_right_mm",               // NEW
            "odt.margin_top_mm",                 // NEW
            "odt.margin_bottom_mm",              // NEW
            "odt.page_break",                    // NEW
            "odt.max_empty_lines"                // NEW
    };
}


// ============================================================
// Parse scalar keys from restricted YAML format
// ============================================================
QStringList ConfigManager::listScalarKeys() const
{
    // Schema v1.1:
    //  - Unlimited depth (limit 8)
    //  - Lists allowed
    //  - We only collect scalar keys
    //  - Return full dot path: a.b.c

    QStringList out;

    QStringList sectionStack;   // tracks nesting
    QVector<int> indentStack;   // tracks indent levels

    for (const QString &raw : m_lines)
    {
        const QString trimmed = raw.trimmed();

        if (trimmed.isEmpty() || trimmed.startsWith('#'))
            continue;

        // Skip list items
        if (trimmed.startsWith('-'))
            continue;

        // Count indent
        int indent = 0;
        while (indent < raw.size() && raw[indent].isSpace())
            indent++;

        // Limit depth to prevent insane YAML
        if (indent / 2 > 8)
            continue;

        const int colon = raw.indexOf(':', indent);
        if (colon < 0)
            continue;

        QString key = raw.mid(indent, colon - indent).trimmed();
        QString after = raw.mid(colon + 1).trimmed();

        if (key.isEmpty())
            continue;

        // Adjust stack according to indent
        while (!indentStack.isEmpty() && indentStack.last() >= indent)
        {
            indentStack.removeLast();
            sectionStack.removeLast();
        }

        // Push current key
        indentStack.append(indent);
        sectionStack.append(key);

        // If this is scalar (has value)
        if (!after.isEmpty())
        {
            out.append(sectionStack.join('.'));
        }
    }

    return out;
}



// ============================================================
// Validate value for known keys (enums / ranges / formats)
// ============================================================
bool ConfigManager::validateConfigStructure()
{
    QMutexLocker lock(&m_mutex);

    m_validationFailed = false;
    bool ok = true;

    // --------------------------------------------------------
    // Schema v1.1 policy:
    //   - YAML depth allowed (handled by listScalarKeys(), limit 8)
    //   - Lists allowed
    //   - Strict validation ONLY for:
    //       config.version
    //       logging.*
    //       ui.*
    //       general.*
    //       recognition.*
    //       odt.*
    //   - Unknown TOP-LEVEL BLOCKS are forbidden
    //   - Legacy TOP-LEVEL SCALARS are allowed ONLY if whitelisted
    // --------------------------------------------------------

    const QSet<QString> strictBlocks = {
        "config",
        "logging",
        "ui",
        "general",
        "recognition",
        "odt"
    };

    // Allowed non-strict blocks present in your real config.yaml
    const QSet<QString> allowedNonStrictBlocks = {
        "threading",
        "preprocess",
        "ocr",
        "tsv_quality",
        "tsv",
        "document",
        "theme",
        "classification",
        "export",
        "styles"
    };

    // Legacy top-level scalars (existing real-world config keys)
    const QSet<QString> allowedTopLevelScalars = {
        "program_root",
        "mode",
        "base_dir",

        "gap_empty_threshold",

        "paragraph_indent_min",
        "paragraph_indent_max",
        "paragraph_continue_max",
        "paragraph_indent_spaces",

        "definition_left_min",
        "definition_left_max",
        "definition_right_min",
        "definition_right_max",
        "definition_gap_threshold",
        "definition_gap_min"
    };

    const QSet<QString> allowedStrictScalarKeys = allowedScalarKeys();
    const QStringList scalarKeys = listScalarKeys();

    // --------------------------------------------------------
    // 1) Validate top-level entities
    //    - "a.b.c"  => top-level block = "a"
    //    - "a"      => top-level scalar (legacy)
    // --------------------------------------------------------
    for (const QString &k : scalarKeys)
    {
        const bool isTopLevelScalar = !k.contains('.');

        if (isTopLevelScalar)
        {
            if (!allowedTopLevelScalars.contains(k))
            {
                LogRouter::instance().error(
                    QString("[ConfigManager] Unknown top-level scalar key: '%1'").arg(k));
                ok = false;
            }
            continue;
        }

        const QString top = k.section('.', 0, 0);

        if (strictBlocks.contains(top))
            continue;

        if (!allowedNonStrictBlocks.contains(top))
        {
            LogRouter::instance().error(
                QString("[ConfigManager] Unknown top-level block: '%1'").arg(top));
            ok = false;
        }
    }

    // --------------------------------------------------------
    // 2) Strict validation for critical scalar keys (ONLY)
    // --------------------------------------------------------
    for (const QString &k : scalarKeys)
    {
        const QString top = k.section('.', 0, 0);

        if (!strictBlocks.contains(top))
            continue;

        // Strict block: only known scalar keys are allowed
        if (!allowedStrictScalarKeys.contains(k))
        {
            LogRouter::instance().error(
                QString("[ConfigManager] Unknown config key in strict block: '%1'").arg(k));
            ok = false;
            continue;
        }

        const QString val = get(k, "").toString().trimmed();
        if (!val.isEmpty() && !validateValueForKey(k, val))
        {
            LogRouter::instance().error(
                QString("[ConfigManager] Invalid value for '%1': '%2'").arg(k, val));
            ok = false;
        }
    }

    if (!ok)
    {
        LogRouter::instance().error("[ConfigManager] Config validation FAILED (schema v1.1).");
        m_validationFailed = true;
    }

    return ok;
}




// ============================================================
// Strict hierarchical read by dot-separated path
// ============================================================
QVariant ConfigManager::get(const QString &path,
                            const QVariant &defaultValue) const
{
    QMutexLocker lock(&m_mutex);

    const QStringList parts = path.split('.');
    if (parts.isEmpty())
        return defaultValue;

    int expectedIndent = 0;
    int searchStartLine = 0;

    for (int level = 0; level < parts.size(); ++level)
    {
        const QString key = parts[level] + ":";
        bool found = false;

        for (int i = searchStartLine; i < m_lines.size(); ++i)
        {
            const QString &raw = m_lines[i];

            if (raw.trimmed().isEmpty() || raw.trimmed().startsWith('#'))
                continue;

            int indent = 0;
            while (indent < raw.size() && raw[indent].isSpace())
                indent++;

            if (indent < expectedIndent)
                break;

            if (indent != expectedIndent)
                continue;

            if (!raw.mid(indent).startsWith(key))
                continue;

            // Found matching level
            found = true;

            if (level == parts.size() - 1)
            {
                QString val = raw.mid(indent + key.length()).trimmed();

                int commentPos = val.indexOf('#');
                if (commentPos >= 0)
                    val = val.left(commentPos).trimmed();

                if ((val.startsWith('"') && val.endsWith('"')) ||
                    (val.startsWith('\'') && val.endsWith('\'')))
                {
                    val = val.mid(1, val.length() - 2);
                }

                return val.isEmpty() ? defaultValue : val;
            }

            // Move to next level
            expectedIndent += 2;
            searchStartLine = i + 1;
            break;
        }

        if (!found)
            return defaultValue;
    }

    return defaultValue;
}


// ============================================================
// Ensure key exists with strict YAML nesting (schema v1.0)
//
// Supported shape ONLY:
//   section:
//     key: value
//
// Depth > 2 is forbidden by design.
// ============================================================
bool ConfigManager::ensureKeyExists(const QString &path,
                                    const QVariant &defaultValue)
{
    QMutexLocker lock(&m_mutex);

    if (get(path).isValid())
        return true;

    // --------------------------------------------------------
    // Production policy: NO auto-insertion
    // --------------------------------------------------------
    if (m_mode == Mode::Production)
    {
        const QString msg =
            QString("[ConfigManager] Production: ensureKeyExists() forbidden for '%1'").arg(path);

        LogRouter::instance().warning(msg);
        recordMigration(msg);
        return false;
    }

    const QStringList parts = path.split('.');
    if (parts.isEmpty())
        return false;

    // --------------------------------------------------------
    // Schema v1.0 restriction: max depth = 2
    // --------------------------------------------------------
    if (parts.size() > 2)
    {
        const QString msg =
            QString("[ConfigManager] ensureKeyExists() rejected (depth>2): '%1'").arg(path);

        LogRouter::instance().error(msg);
        recordMigration(msg);
        return false;
    }

    const QString section = parts[0];
    const bool wantScalarAtTop = (parts.size() == 1);

    // --------------------------------------------------------
    // 1) Find existing section line: "section:" at indent 0
    //    and determine its block range [secLine+1 .. nextTopLevel-1]
    // --------------------------------------------------------
    int secLine = -1;
    for (int i = 0; i < m_lines.size(); ++i)
    {
        const QString raw = m_lines[i];
        if (raw.trimmed().isEmpty() || raw.trimmed().startsWith('#'))
            continue;

        // indent must be 0
        int indent = 0;
        while (indent < raw.size() && raw[indent].isSpace())
            indent++;

        if (indent != 0)
            continue;

        if (raw.mid(indent).startsWith(section + ":"))
        {
            secLine = i;
            break;
        }
    }

    // If no section exists, create it at end.
    if (secLine < 0)
    {
        // Create top-level section
        if (wantScalarAtTop)
        {
            m_lines.append(section + ": " + buildYamlValue(defaultValue));
        }
        else
        {
            m_lines.append(section + ":");
            m_lines.append(QString("  %1: %2")
                               .arg(parts[1])
                               .arg(buildYamlValue(defaultValue)));
        }

        recordMigration(QString("[ConfigManager] Added missing key '%1'").arg(path));
        return true;
    }

    // Determine end of section block
    int secEnd = m_lines.size();
    for (int i = secLine + 1; i < m_lines.size(); ++i)
    {
        const QString raw = m_lines[i];
        if (raw.trimmed().isEmpty() || raw.trimmed().startsWith('#'))
            continue;

        int indent = 0;
        while (indent < raw.size() && raw[indent].isSpace())
            indent++;

        // next top-level section begins
        if (indent == 0)
        {
            secEnd = i;
            break;
        }
    }

    // --------------------------------------------------------
    // 2) If we want scalar at top-level (rare), replace/insert
    //    but for schema v1.0 we normally keep section as mapping.
    // --------------------------------------------------------
    if (wantScalarAtTop)
    {
        // Convert "section:" -> "section: value"
        // Keep possible inline comment
        QString &raw = m_lines[secLine];

        QString inlineComment;
        const int commentPos = raw.indexOf('#');
        if (commentPos >= 0)
            inlineComment = "  " + raw.mid(commentPos);

        raw = section + ": " + buildYamlValue(defaultValue) + inlineComment;

        recordMigration(QString("[ConfigManager] Added missing key '%1'").arg(path));
        return true;
    }

    // --------------------------------------------------------
    // 3) Insert missing child key inside the section block.
    //    Insert before secEnd (before next top-level section).
    // --------------------------------------------------------
    const QString child = parts[1];
    const QString childPrefix = child + ":";

    // Check if child exists already in block (indent 2)
    for (int i = secLine + 1; i < secEnd; ++i)
    {
        const QString raw = m_lines[i];
        if (raw.trimmed().isEmpty() || raw.trimmed().startsWith('#'))
            continue;

        int indent = 0;
        while (indent < raw.size() && raw[indent].isSpace())
            indent++;

        if (indent != 2)
            continue;

        if (raw.mid(indent).startsWith(childPrefix))
            return true; // already exists
    }

    // Insert new child line at the end of the section block
    m_lines.insert(secEnd,
                   QString("  %1: %2")
                       .arg(child)
                       .arg(buildYamlValue(defaultValue)));

    recordMigration(QString("[ConfigManager] Added missing key '%1'").arg(path));
    return true;
}



// ============================================================
// Strict hierarchical update by dot-separated path
// ============================================================
bool ConfigManager::set(const QString &path, const QVariant &value)
{
    QMutexLocker lock(&m_mutex);

    const QStringList parts = path.split('.');
    if (parts.isEmpty())
        return false;

    int expectedIndent = 0;
    int searchStartLine = 0;

    for (int level = 0; level < parts.size(); ++level)
    {
        const QString key = parts[level] + ":";
        bool found = false;

        for (int i = searchStartLine; i < m_lines.size(); ++i)
        {
            QString &raw = m_lines[i];

            if (raw.trimmed().isEmpty() || raw.trimmed().startsWith('#'))
                continue;

            int indent = 0;
            while (indent < raw.size() && raw[indent].isSpace())
                indent++;

            if (indent < expectedIndent)
                break;

            if (indent != expectedIndent)
                continue;

            if (!raw.mid(indent).startsWith(key))
                continue;

            found = true;

            if (level == parts.size() - 1)
            {
                QString inlineComment;
                int commentPos = raw.indexOf('#');
                if (commentPos >= 0)
                    inlineComment = "  " + raw.mid(commentPos);

                raw = QString(indent, ' ')
                      + key
                      + " "
                      + buildYamlValue(value)
                      + inlineComment;

                return true;
            }

            expectedIndent += 2;
            searchStartLine = i + 1;
            break;
        }

        if (!found)
        {
            // Insert safely if missing
            if (!found)
            {
                if (m_mode == Mode::Production)
                {
                    const QString msg =
                        QString("[ConfigManager] Production: set() failed, missing key '%1'").arg(path);
                    LogRouter::instance().warning(msg);
                    recordMigration(msg);
                    return false;
                }

                return ensureKeyExists(path, value);
            }

        }
    }

    return false;
}


// ============================================================
// Automatic key migration
// ============================================================
void ConfigManager::migrate()
{
    QMutexLocker lock(&m_mutex);

    // --------------------------------------------------------
    // Production policy:
    //   - DO NOT modify config.yaml implicitly
    //   - Only record what would be migrated
    // --------------------------------------------------------
    const bool production = (m_mode == Mode::Production);

    const int version = get("config.version", 0).toInt();

    if (version < 1)
    {
        if (production)
        {
            recordMigration("[ConfigManager] Production: would set config.version=1");
            // Do NOT write.
        }
        else
        {
            set("config.version", 1);
            recordMigration("[ConfigManager] Migrated config.version -> 1");
        }
    }


    auto wouldSet = [&](const QString &key, const QVariant &val)
    {
        const QString msg =
            QString("[ConfigManager] Production: would migrate set '%1'='%2'")
                .arg(key)
                .arg(val.toString());

        recordMigration(msg);
    };

    auto wouldEnsure = [&](const QString &key, const QVariant &def)
    {
        const QString msg =
            QString("[ConfigManager] Production: would ensure key '%1' (default='%2')")
                .arg(key)
                .arg(def.toString());

        recordMigration(msg);
    };

    // UI / Theme migration
    const QString oldTheme = get("ui.theme", "").toString();
    const QString newMode  = get("ui.theme_mode", "").toString();

    if (!oldTheme.isEmpty() && newMode.isEmpty())
    {
        if (production) wouldSet("ui.theme_mode", oldTheme);
        else {
            set("ui.theme_mode", oldTheme);
            recordMigration(QString("[ConfigManager] Migrated ui.theme -> ui.theme_mode (%1)").arg(oldTheme));
        }
    }

    // Default UI keys
    struct K { const char* key; QVariant def; };
    const K keys[] = {
                      {"ui.custom_qss", ""},
                      {"ui.app_font_family", ""},
                      {"ui.app_font_size", 11},
                      {"ui.log_font_size", 10},
                      {"ui.toolbar_style", "icons"},
                      {"ui.thumbnail_size", 160},
                      {"ui.notify_on_finish", true},
                      {"ui.play_sound_on_finish", true},
                      {"ui.sound_volume", 70},
                      {"ui.sound_path", "sounds/done.wav"},
                      };


    for (const auto &k : keys)
    {
        const QString key = QString::fromUtf8(k.key);
        const QVariant exists = get(key, QVariant());
        const bool present = exists.isValid();

        if (!present)
        {
            if (production) wouldEnsure(key, k.def);
            else ensureKeyExists(key, k.def);
        }
    }
}



// ============================================================
// Save config back to disk (ATOMIC, production-safe)
// ============================================================
bool ConfigManager::save()
{
    QMutexLocker lock(&m_mutex);

    // --------------------------------------------------------
    // Atomic save contract:
    //   - Write to a temporary file first
    //   - Commit (atomic replace) only after full write succeeds
    //   - Never leave partially written config.yaml behind
    //
    // Qt provides QSaveFile exactly for this purpose.
    // --------------------------------------------------------

    if (m_filePath.trimmed().isEmpty())
    {
        LogRouter::instance().error(
            "[ConfigManager] save() failed: empty m_filePath");
        return false;
    }

    // NOTE: QSaveFile writes into <path>.<random>.tmp internally
    // and replaces the target atomically on commit().
    QSaveFile out(m_filePath);

    // Optional but recommended (ensures durable write on some FS)
    out.setDirectWriteFallback(true);

    if (!out.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        LogRouter::instance().error(
            QString("[ConfigManager] Cannot write config file: %1").arg(m_filePath));
        return false;
    }

    QTextStream ts(&out);
    ts.setEncoding(QStringConverter::Utf8);

    for (const QString &line : m_lines)
        ts << line << "\n";

    ts.flush();

    // --------------------------------------------------------
    // Commit = atomic replace
    // If commit fails, original config remains untouched.
    // --------------------------------------------------------
    if (!out.commit())
    {
        LogRouter::instance().error(
            QString("[ConfigManager] Atomic commit failed: %1").arg(m_filePath));
        return false;
    }

    return true;
}


// ============================================================
// Debug dump
// ============================================================
void ConfigManager::dump() const
{
    QMutexLocker lock(&m_mutex);

    // CHANGED: qDebug() spam -> LogRouter::debug (honors logging.level)
    LogRouter::instance().debug("[ConfigManager] Loaded config.yaml:");
    for (const auto &line : m_lines)
        LogRouter::instance().debug(line);
}

// ============================================================
//  Validation state query
// ============================================================
bool ConfigManager::validationFailed() const
{
    return m_validationFailed;
}

// ============================================================
// Default registry (schema v1.1)
// ============================================================
QMap<QString, QVariant> ConfigManager::defaultScalarValues()
{
    // IMPORTANT:
    //  - Every key listed in allowedScalarKeys() must have a default here.
    //  - This registry defines the canonical schema defaults.
    //  - No logic here — only static values.
    //  - Used by resetToDefaults() and migrations.

    return QMap<QString, QVariant>{

        // ----------------------------------------------------
        // Config
        // ----------------------------------------------------
        {"config.version", 1},

            // ----------------------------------------------------
            // Logging
            // ----------------------------------------------------
            {"logging.enabled",        true},
            {"logging.level",          3},
            {"logging.file_output",    false},
            {"logging.gui_output",     true},
            {"logging.console_output", true},
            {"logging.file_path",      "ocrtoodt.log"},
            {"logging.max_file_size_mb", 5},

            // ----------------------------------------------------
            // UI
            // ----------------------------------------------------
            {"ui.theme_mode",          "dark"},
            {"ui.custom_qss",          ""},
            {"ui.app_font_family",     ""},
            {"ui.app_font_size",       12},
            {"ui.font_size",           12},   // NEW (schema-level)
            {"ui.log_font_size",       10},
            {"ui.toolbar_style",       "icons"},
            {"ui.toolbar_icon_size",   24},   // NEW
            {"ui.thumbnail_size",      130},
            {"ui.language",            "en"}, // NEW
            {"ui.expert_mode",         true},
            {"ui.notify_on_finish",    false},
            {"ui.play_sound_on_finish", false},
            {"ui.sound_volume",        80},
            {"ui.sound_path",          "sounds/done.wav"},
            {"ui.show_preprocess_tab", true},
            {"ui.show_logging_tab",    true},

            // ----------------------------------------------------
            // General
            // ----------------------------------------------------
            {"general.parallel_enabled", true},
            {"general.num_processes",    "auto"},
            {"general.mode",             "disk_only"},
            {"general.debug_mode",       true},
            {"general.input_dir",        "input"},
            {"general.preprocess_path",  "preprocess"},
            {"general.output_file",      "output/result.odt"},  // NEW
            {"general.ocr_path",         "cache/ocr"},          // NEW

            // ----------------------------------------------------
            // Preprocess
            // ----------------------------------------------------
            {"preprocess.profile", "pdf_auto"},

            // ----------------------------------------------------
            // Recognition
            // ----------------------------------------------------
            {"recognition.language", "eng"},
            {"recognition.psm",      3},

            // ----------------------------------------------------
            // ODT
            // ----------------------------------------------------
            {"odt.font_family", "Times New Roman"},
            {"odt.font_size",   12},
            {"odt.justify",     true},

            {"odt.paper_size",                 "A4"},        // NEW
            {"odt.font_name",                  "Ubuntu Sans"}, // NEW
            {"odt.font_size_pt",               13},          // NEW
            {"odt.text_align",                 "center"},    // NEW
            {"odt.first_line_indent_mm",       20},          // NEW
            {"odt.paragraph_spacing_after_pt", 3},           // NEW
            {"odt.line_height_percent",        100},         // NEW
            {"odt.margin_left_mm",             20},          // NEW
            {"odt.margin_right_mm",            15},          // NEW
            {"odt.margin_top_mm",              20},          // NEW
            {"odt.margin_bottom_mm",           15},          // NEW
            {"odt.page_break",                 true},        // NEW
            {"odt.max_empty_lines",            1}            // NEW
    };
}

// ============================================================
// Schema sync guard (Debug-only)
// Ensures allowedScalarKeys() == defaultScalarValues().keys()
// ============================================================
#ifndef QT_NO_DEBUG
void ConfigManager::debugAssertSchemaRegistrySyncOnce()
{
    static bool done = false;
    if (done)
        return;
    done = true;

    const QSet<QString> allowed = allowedScalarKeys();

    const QMap<QString, QVariant> defaultsMap = defaultScalarValues();
    const QSet<QString> defaults =
        QSet<QString>(defaultsMap.keyBegin(), defaultsMap.keyEnd());

    if (allowed == defaults)
        return;

    const QSet<QString> missingInDefaults = allowed - defaults;
    const QSet<QString> extraInDefaults   = defaults - allowed;

    if (!missingInDefaults.isEmpty())
    {
        LogRouter::instance().error(
            QString("[ConfigManager] Schema mismatch: missing in defaultScalarValues(): %1")
                .arg(QStringList(missingInDefaults.begin(), missingInDefaults.end()).join(", ")));
    }

    if (!extraInDefaults.isEmpty())
    {
        LogRouter::instance().error(
            QString("[ConfigManager] Schema mismatch: extra in defaultScalarValues(): %1")
                .arg(QStringList(extraInDefaults.begin(), extraInDefaults.end()).join(", ")));
    }

    Q_ASSERT_X(false, "ConfigManager",
               "Schema registry mismatch: allowedScalarKeys() != defaultScalarValues().keys()");
}
#endif



// ============================================================
// Reset config.yaml to canonical defaults (schema v1.0)
// ============================================================
bool ConfigManager::resetToDefaults()
{
    QMutexLocker lock(&m_mutex);

    if (m_filePath.trimmed().isEmpty())
    {
        LogRouter::instance().error(
            "[ConfigManager] resetToDefaults() failed: no file loaded.");
        return false;
    }

    LogRouter::instance().warning(
        "[ConfigManager] Resetting configuration to defaults.");

    // --------------------------------------------------------
    // Temporarily allow insertion even in Production mode
    // --------------------------------------------------------
    const Mode previousMode = m_mode;
    m_mode = Mode::Development;

    const QMap<QString, QVariant> defaults = defaultScalarValues();

    for (auto it = defaults.begin(); it != defaults.end(); ++it)
    {
        const QString key = it.key();
        const QVariant val = it.value();

        set(key, val);
    }

    // Restore original mode
    m_mode = previousMode;

    // --------------------------------------------------------
    // Atomic save
    // --------------------------------------------------------
    if (!save())
    {
        LogRouter::instance().error(
            "[ConfigManager] resetToDefaults() failed during save().");
        return false;
    }

    // --------------------------------------------------------
    // Reload to ensure canonical state
    // --------------------------------------------------------
    lock.unlock();  // avoid recursive lock during reload()

    if (!reload())
    {
        LogRouter::instance().error(
            "[ConfigManager] resetToDefaults() failed during reload().");
        return false;
    }

    LogRouter::instance().info(
        "[ConfigManager] Configuration successfully reset to defaults.");

    return true;
}

// ============================================================
//  OCRtoODT — Config Manager: Import / Export
//  File: core/configmanager.cpp
//
//  Responsibility:
//      - Export current loaded config to an external file
//      - Import external config into active config.yaml safely
//
//  Safety policy:
//      - Import validates schema v1.0 before applying
//      - Import creates timestamped backup before overwrite
//      - Writes are atomic (QSaveFile)
// ============================================================

// ------------------------------------------------------------
// Export current in-memory config to an arbitrary file
// ------------------------------------------------------------
bool ConfigManager::exportToFile(const QString &path) const
{
    QMutexLocker lock(&m_mutex);

    if (path.trimmed().isEmpty())
    {
        LogRouter::instance().error("[ConfigManager] exportToFile() failed: empty path");
        return false;
    }

    // Ensure parent folder exists
    const QString folder = QFileInfo(path).absolutePath();
    if (!folder.isEmpty())
        QDir().mkpath(folder);

    QSaveFile out(path);
    out.setDirectWriteFallback(true);

    if (!out.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        LogRouter::instance().error(
            QString("[ConfigManager] exportToFile() cannot write: %1").arg(path));
        return false;
    }

    QTextStream ts(&out);
    ts.setEncoding(QStringConverter::Utf8);

    for (const QString &line : m_lines)
        ts << line << "\n";

    ts.flush();

    if (!out.commit())
    {
        LogRouter::instance().error(
            QString("[ConfigManager] exportToFile() atomic commit failed: %1").arg(path));
        return false;
    }

    LogRouter::instance().info(
        QString("[ConfigManager] Exported config to: %1").arg(path));

    return true;
}

// ------------------------------------------------------------
// Helper: create backup of current active config.yaml
// ------------------------------------------------------------
static bool backupFile(const QString &srcPath, QString *outBackupPath)
{
    if (!QFileInfo::exists(srcPath))
        return true; // nothing to backup

    const QString ts = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
    const QString backupPath = srcPath + "." + ts + ".bak";

    // Copy is okay here — backup does not need atomicity.
    if (!QFile::copy(srcPath, backupPath))
        return false;

    if (outBackupPath)
        *outBackupPath = backupPath;

    return true;
}

// ------------------------------------------------------------
// Import external config into current active config.yaml (m_filePath)
// ------------------------------------------------------------
bool ConfigManager::importFromFile(const QString &path)
{
    // NOTE:
    // We do not hold lock across the whole flow because validateConfigStructure()
    // and reload() may re-enter. We do a staged approach.

    // 1) Read file lines (no modifications)
    QStringList importedLines;
    {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            LogRouter::instance().error(
                QString("[ConfigManager] importFromFile() cannot open: %1").arg(path));
            return false;
        }

        QTextStream ts(&f);
        ts.setEncoding(QStringConverter::Utf8);

        while (!ts.atEnd())
            importedLines.append(ts.readLine());
    }

    // 2) Validate imported config using current validator.
    //    Strategy: temporarily swap m_lines, validate, then restore if failed.
    QString activePathCopy;
    QStringList oldLines;
    {
        QMutexLocker lock(&m_mutex);

        activePathCopy = m_filePath;
        if (activePathCopy.trimmed().isEmpty())
        {
            LogRouter::instance().error(
                "[ConfigManager] importFromFile() failed: no active config path (m_filePath empty)");
            return false;
        }

        oldLines = m_lines;
        m_lines = importedLines;

        // IMPORTANT:
        // validateConfigStructure() must set internal validation state
        // (you already have m_validationFailed + validationFailed()).
        const bool ok = validateConfigStructure();

        if (!ok)
        {
            // Restore old config in memory (import rejected)
            m_lines = oldLines;

            LogRouter::instance().error(
                QString("[ConfigManager] importFromFile() rejected: validation failed for %1").arg(path));
            return false;
        }

        // Imported lines are valid; keep them in memory for now.
    }

    // 3) Backup current active config.yaml
    QString backupPath;
    if (!backupFile(activePathCopy, &backupPath))
    {
        LogRouter::instance().error(
            QString("[ConfigManager] importFromFile() failed: could not create backup for %1").arg(activePathCopy));
        return false;
    }

    if (!backupPath.isEmpty())
    {
        LogRouter::instance().warning(
            QString("[ConfigManager] Backup created: %1").arg(backupPath));
    }

    // 4) Write imported lines into active config.yaml atomically
    {
        QSaveFile out(activePathCopy);
        out.setDirectWriteFallback(true);

        if (!out.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            LogRouter::instance().error(
                QString("[ConfigManager] importFromFile() cannot write active config: %1").arg(activePathCopy));
            return false;
        }

        QTextStream ts(&out);
        ts.setEncoding(QStringConverter::Utf8);

        // Re-read m_lines under lock to guarantee consistency
        QStringList linesToWrite;
        {
            QMutexLocker lock(&m_mutex);
            linesToWrite = m_lines;
        }

        for (const QString &line : linesToWrite)
            ts << line << "\n";

        ts.flush();

        if (!out.commit())
        {
            LogRouter::instance().error(
                QString("[ConfigManager] importFromFile() atomic commit failed: %1").arg(activePathCopy));
            return false;
        }
    }

    // 5) Reload active config into memory (this also runs migrate + validate)
    if (!reload())
    {
        LogRouter::instance().error(
            "[ConfigManager] importFromFile() failed: reload() failed after import");
        return false;
    }

    LogRouter::instance().info(
        QString("[ConfigManager] Imported config from %1 into %2").arg(path, activePathCopy));

    return true;
}

// ============================================================
// Validate value for known keys (enums / ranges / formats)
// ============================================================
bool ConfigManager::validateValueForKey(const QString &fullKey,
                                        const QString &rawValue) const
{
    auto isBool = [&](const QString &v)
    {
        return v == "true" || v == "false";
    };

    auto toInt = [&](int min, int max)
    {
        bool ok = false;
        int v = rawValue.toInt(&ok);
        return ok && v >= min && v <= max;
    };

    auto toDouble = [&](double min, double max)
    {
        bool ok = false;
        double v = rawValue.toDouble(&ok);
        return ok && v >= min && v <= max;
    };

    // --------------------------------------------------------
    // CONFIG
    // --------------------------------------------------------
    if (fullKey == "config.version")
        return toInt(1, 999);

    // --------------------------------------------------------
    // LOGGING
    // --------------------------------------------------------
    if (fullKey.startsWith("logging."))
    {
        if (fullKey == "logging.enabled" ||
            fullKey == "logging.file_output" ||
            fullKey == "logging.gui_output" ||
            fullKey == "logging.console_output")
            return isBool(rawValue);

        if (fullKey == "logging.level")
            return toInt(0, 4);

        if (fullKey == "logging.max_file_size_mb")
            return (rawValue == "1" || rawValue == "2" || rawValue == "5");

        return true;
    }

    // --------------------------------------------------------
    // UI
    // --------------------------------------------------------
    if (fullKey == "ui.theme_mode")
    {
        QString v = rawValue.toLower();
        return v == "light" || v == "dark" || v == "auto" || v == "system";
    }

    if (fullKey == "ui.toolbar_style")
    {
        QString v = rawValue.toLower();
        return v == "icons" || v == "text" || v == "icons_text";
    }

    if (fullKey == "ui.toolbar_icon_size")
        return toInt(12, 64);

    if (fullKey == "ui.thumbnail_size")
        return toInt(80, 400);

    if (fullKey == "ui.app_font_size" ||
        fullKey == "ui.font_size" ||
        fullKey == "ui.log_font_size")
        return toInt(6, 72);

    if (fullKey == "ui.sound_volume")
        return toInt(0, 100);

    if (fullKey == "ui.expert_mode" ||
        fullKey == "ui.notify_on_finish" ||
        fullKey == "ui.play_sound_on_finish" ||
        fullKey == "ui.show_preprocess_tab" ||
        fullKey == "ui.show_logging_tab")
        return isBool(rawValue);

    if (fullKey == "ui.language")
        return rawValue.length() >= 2 && rawValue.length() <= 8;

    // --------------------------------------------------------
    // GENERAL
    // --------------------------------------------------------
    if (fullKey == "general.parallel_enabled" ||
        fullKey == "general.debug_mode")
        return isBool(rawValue);

    if (fullKey == "general.num_processes")
    {
        if (rawValue == "auto")
            return true;
        return toInt(1, 256);
    }

    if (fullKey == "general.mode")
    {
        QString v = rawValue.toLower();
        return v == "auto" || v == "ram_only" || v == "disk_only";
    }

    // paths — allow any non-empty scalar
    if (fullKey == "general.input_dir" ||
        fullKey == "general.preprocess_path" ||
        fullKey == "general.output_file" ||
        fullKey == "general.ocr_path")
        return true;

    // --------------------------------------------------------
    // RECOGNITION
    // --------------------------------------------------------
    if (fullKey == "recognition.psm")
        return toInt(0, 13);

    if (fullKey == "recognition.language")
        return rawValue.length() >= 2;

    // --------------------------------------------------------
    // ODT
    // --------------------------------------------------------
    if (fullKey == "odt.justify" ||
        fullKey == "odt.page_break")
        return isBool(rawValue);

    if (fullKey == "odt.paper_size")
    {
        QString v = rawValue.toUpper();
        return v == "A4" || v == "A5" || v == "LETTER";
    }

    if (fullKey == "odt.text_align")
    {
        QString v = rawValue.toLower();
        return v == "left" || v == "center" || v == "right" || v == "justify";
    }

    if (fullKey == "odt.font_size" ||
        fullKey == "odt.font_size_pt")
        return toInt(6, 72);

    if (fullKey == "odt.first_line_indent_mm")
        return toInt(0, 100);

    if (fullKey == "odt.paragraph_spacing_after_pt")
        return toInt(0, 100);

    if (fullKey == "odt.line_height_percent")
        return toInt(50, 300);

    if (fullKey == "odt.margin_left_mm" ||
        fullKey == "odt.margin_right_mm" ||
        fullKey == "odt.margin_top_mm" ||
        fullKey == "odt.margin_bottom_mm")
        return toInt(0, 100);

    if (fullKey == "odt.max_empty_lines")
        return toInt(0, 5);

    // default: allow scalar
    return true;
}
