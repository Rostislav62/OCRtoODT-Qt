// ============================================================
//  OCRtoODT — Config Manager (Hierarchical, Comment-Preserving)
//  File: core/configmanager.cpp
//  NEW
// ============================================================

#include "ConfigManager.h"

#include "core/LogRouter.h" // CHANGED: unified logging via LogRouter

#include <QFile>
#include <QTextStream>
#include <QStringConverter>

// ============================================================
// Singleton
// ============================================================
ConfigManager &ConfigManager::instance()
{
    static ConfigManager inst;
    return inst;
}

// ============================================================
// Load YAML file into raw text buffer
// ============================================================
bool ConfigManager::load(const QString &path)
{
    m_filePath = path;
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

    return true;
}

// ============================================================
// Reload config.yaml from the last loaded path
// ============================================================
bool ConfigManager::reload()
{
    if (m_filePath.trimmed().isEmpty())
    {
        // CHANGED: qWarning() -> LogRouter
        LogRouter::instance().warning(
            "[ConfigManager] reload() called but no file was loaded yet.");
        return false;
    }

    return load(m_filePath);
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
// Read hierarchical value by dot-separated path
// ============================================================
QVariant ConfigManager::get(const QString &path,
                            const QVariant &defaultValue) const
{
    const QStringList parts = path.split('.');
    if (parts.isEmpty())
        return defaultValue;

    int currentLevel = 0;
    int baseIndent = -1;

    for (int i = 0; i < m_lines.size(); ++i)
    {
        const QString &raw = m_lines[i];
        if (raw.trimmed().isEmpty() || raw.trimmed().startsWith('#'))
            continue;

        int indent = 0;
        while (indent < raw.size() && raw[indent].isSpace())
            indent++;

        QString trimmed = raw.trimmed();

        if (baseIndent >= 0 && indent <= baseIndent)
        {
            currentLevel = 0;
            baseIndent = -1;
        }

        if (currentLevel >= parts.size())
            break;

        const QString expectedKey = parts[currentLevel] + ":";

        if (trimmed.startsWith(expectedKey))
        {
            if (currentLevel == 0)
                baseIndent = indent;

            if (currentLevel == parts.size() - 1)
            {
                QString val = trimmed.mid(expectedKey.length()).trimmed();

                int c = val.indexOf('#');
                if (c >= 0)
                    val = val.left(c).trimmed();

                if ((val.startsWith('"') && val.endsWith('"')) ||
                    (val.startsWith('\'') && val.endsWith('\'')))
                {
                    val = val.mid(1, val.length() - 2);
                }

                return val.isEmpty() ? defaultValue : val;
            }

            currentLevel++;
        }
    }

    return defaultValue;
}

// ============================================================
// Ensure key exists (minimal insertion)
// ============================================================
bool ConfigManager::ensureKeyExists(const QString &path,
                                    const QVariant &defaultValue)
{
    if (get(path).isValid())
        return true;

    const QStringList parts = path.split('.');
    if (parts.isEmpty())
        return false;

    int insertAt = m_lines.size();
    int indent = 0;

    for (int level = 0; level < parts.size(); ++level)
    {
        bool found = false;

        for (int i = 0; i < m_lines.size(); ++i)
        {
            QString trimmed = m_lines[i].trimmed();
            if (trimmed.startsWith(parts[level] + ":"))
            {
                insertAt = i + 1;
                found = true;
                break;
            }
        }

        if (!found)
        {
            QString line(indent, ' ');
            line += parts[level] + ":";

            if (level == parts.size() - 1)
                line += " " + buildYamlValue(defaultValue);

            m_lines.insert(insertAt, line);
            insertAt++;
        }

        indent += 2;
    }

    return true;
}

// ============================================================
// Update key but preserve comment and indentation
// ============================================================
bool ConfigManager::set(const QString &path, const QVariant &value)
{
    const QStringList parts = path.split('.');
    if (parts.isEmpty())
        return false;

    int currentLevel = 0;
    int baseIndent = -1;

    for (int i = 0; i < m_lines.size(); ++i)
    {
        QString &raw = m_lines[i];
        QString trimmed = raw.trimmed();

        if (trimmed.isEmpty() || trimmed.startsWith('#'))
            continue;

        int indent = 0;
        while (indent < raw.size() && raw[indent].isSpace())
            indent++;

        if (baseIndent >= 0 && indent <= baseIndent)
        {
            currentLevel = 0;
            baseIndent = -1;
        }

        if (currentLevel >= parts.size())
            break;

        const QString expectedKey = parts[currentLevel] + ":";

        if (trimmed.startsWith(expectedKey))
        {
            if (currentLevel == 0)
                baseIndent = indent;

            if (currentLevel == parts.size() - 1)
            {
                QString inlineComment;
                int commentPos = trimmed.indexOf('#');
                if (commentPos >= 0)
                    inlineComment = "  " + trimmed.mid(commentPos);

                raw = QString(indent, ' ')
                      + parts[currentLevel]
                      + ": "
                      + buildYamlValue(value)
                      + inlineComment;

                return true;
            }

            currentLevel++;
        }
    }

    // If not found — insert minimally
    return ensureKeyExists(path, value);
}

// ============================================================
// Automatic key migration
// ============================================================
void ConfigManager::migrate()
{
    // UI / Theme migration (existing logic preserved)

    QString oldTheme = get("ui.theme", "").toString();
    QString newMode  = get("ui.theme_mode", "").toString();

    if (!oldTheme.isEmpty() && newMode.isEmpty())
        set("ui.theme_mode", oldTheme);

    ensureKeyExists("ui.custom_qss", "");
    ensureKeyExists("ui.app_font_family", "");
    ensureKeyExists("ui.app_font_size", 11);
    ensureKeyExists("ui.font_size", 11);
    ensureKeyExists("ui.log_font_size", 10);
    ensureKeyExists("ui.toolbar_style", "icons");
    ensureKeyExists("ui.thumbnail_size", 160);

    ensureKeyExists("ui.notify_on_finish", true);
    ensureKeyExists("ui.play_sound_on_finish", true);
    ensureKeyExists("ui.sound_volume", 70);
    ensureKeyExists("ui.sound_path", "sounds/done.wav");
}

// ============================================================
// Save config back to disk
// ============================================================
bool ConfigManager::save()
{
    QFile f(m_filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        // CHANGED: qWarning() -> LogRouter
        LogRouter::instance().error(
            QString("[ConfigManager] Cannot write config file: %1").arg(m_filePath));
        return false;
    }

    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);

    for (const QString &line : m_lines)
        ts << line << "\n";

    return true;
}

// ============================================================
// Debug dump
// ============================================================
void ConfigManager::dump() const
{
    // CHANGED: qDebug() spam -> LogRouter::debug (honors logging.level)
    LogRouter::instance().debug("[ConfigManager] Loaded config.yaml:");
    for (const auto &line : m_lines)
        LogRouter::instance().debug(line);
}
