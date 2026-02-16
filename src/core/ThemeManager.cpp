// ============================================================
//  OCRtoODT — Theme Manager 2.0 (+ Token Engine)
//  File: thememanager.cpp
//
//  Responsibility:
//      - Apply theme, fonts, toolbar style, thumbnails and language
//      - Centralized UI appearance control
//
//  Localization policy:
//      - This file contains NO user-facing UI strings
//      - All QString literals here are technical/logging messages
//      - They MUST NOT be wrapped in tr()
//      - They MUST NOT be included in .ts files
//
//  Logging audit result:
//      ✔ All qDebug / qWarning removed
//      ✔ Unified logging via LogRouter
//
// ============================================================


#include "ThemeManager.h"

#include "core/ConfigManager.h"
#include "core/LogRouter.h"

#include <QApplication>
#include <QFile>
#include <QTime>
#include <QTextStream>
#include <QStringConverter>
#include <QDir>
#include <QFileInfo>

// ============================================================
//  Singleton
// ============================================================
ThemeManager *ThemeManager::instance()
{
    static ThemeManager *s_inst = new ThemeManager(qApp);
    return s_inst;
}

// ============================================================
//  Constructor
// ============================================================
ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
    if (qApp) {
        m_appFont = qApp->font();
        m_logFont = qApp->font();
    }
}

// ============================================================
//  Public: apply everything from config.yaml
// ============================================================
void ThemeManager::applyAllFromConfig()
{
    loadStateFromConfig();
    applyThemeToApplication();
    applyFontsToApplication();

    emit themeApplied(m_mode);
    emit fontsApplied(m_appFont, m_logFont);
    emit toolbarStyleChanged(m_toolbarStyle);
    emit thumbnailSizeChanged(m_thumbnailSize);

    LogRouter::instance().info(
        QString("[ThemeManager] Theme applied: mode=%1 toolbar=%2 thumb=%3")
            .arg(static_cast<int>(m_mode))
            .arg(m_toolbarStyle)
            .arg(m_thumbnailSize));
}

// ============================================================
//  Slot: reload settings and re-apply
// ============================================================
void ThemeManager::reloadFromSettings()
{
    LogRouter::instance().debug("[ThemeManager] Reloading theme from settings");
    applyAllFromConfig();
}

// ============================================================
//  Internal: read all relevant keys from ConfigManager
// ============================================================
void ThemeManager::loadStateFromConfig()
{
    ConfigManager &cfg = ConfigManager::instance();

    QString modeStr = cfg.get("ui.theme_mode", "dark").toString().toLower();
    QString custom  = cfg.get("ui.custom_qss", "").toString().trimmed();

    m_mode          = resolveMode(modeStr, custom);
    m_customQssPath = custom;

    QString family = cfg.get("ui.app_font_family", "").toString();
    int appSize    = cfg.get("ui.app_font_size", 11).toInt();
    int logSize    = cfg.get("ui.log_font_size", 10).toInt();

    if (appSize < 6) appSize = 11;
    if (logSize < 6) logSize = 10;

    if (!family.isEmpty()) {
        m_appFont = QFont(family, appSize);
    } else if (qApp) {
        m_appFont = qApp->font();
        m_appFont.setPointSize(appSize);
    }

    m_logFont = m_appFont;
    m_logFont.setPointSize(logSize);

    m_toolbarStyle  = cfg.get("ui.toolbar_style", "icons").toString();
    m_thumbnailSize = cfg.get("ui.thumbnail_size", 160).toInt();

}

// ============================================================
//  Convert string from config.yaml to ThemeMode
// ============================================================
ThemeManager::ThemeMode ThemeManager::resolveMode(
    const QString &modeString,
    const QString &customQssPath) const
{
    if (modeString == "light")   return ThemeMode::Light;
    if (modeString == "dark")    return ThemeMode::Dark;
    if (modeString == "auto")    return ThemeMode::Auto;
    if (modeString == "system")  return ThemeMode::System;
    if (!customQssPath.isEmpty()) return ThemeMode::Custom;
    return ThemeMode::Dark;
}

// ============================================================
//  Resolve "Auto" mode based on time-of-day
// ============================================================
ThemeManager::ThemeMode ThemeManager::resolveAutoMode() const
{
    QTime now = QTime::currentTime();
    if (!now.isValid())
        return ThemeMode::Light;

    return (now.hour() >= 21 || now.hour() < 7)
               ? ThemeMode::Dark
               : ThemeMode::Light;
}

// ============================================================
//  Apply theme: build stylesheet and set it on QApplication
// ============================================================
void ThemeManager::applyThemeToApplication()
{
    if (!qApp)
        return;

    ThemeMode effectiveMode = m_mode;
    if (m_mode == ThemeMode::Auto || m_mode == ThemeMode::System)
        effectiveMode = resolveAutoMode();

    QString baseQss;

    if (m_mode == ThemeMode::Custom && !m_customQssPath.isEmpty()) {
        baseQss = loadCustomQss(m_customQssPath);
        if (baseQss.isEmpty()) {
            LogRouter::instance().warning(
                "[ThemeManager] Custom QSS failed, falling back to dark theme");
            effectiveMode = ThemeMode::Dark;
            baseQss = loadBuiltinQss(effectiveMode);
        }
    } else {
        baseQss = loadBuiltinQss(effectiveMode);
    }

    m_activeStylesheet = buildFinalStylesheet(effectiveMode, baseQss);
    qApp->setStyleSheet(m_activeStylesheet);
}

// ============================================================
//  Apply fonts to QApplication
// ============================================================
void ThemeManager::applyFontsToApplication()
{
    if (qApp)
        qApp->setFont(m_appFont);
}

// ============================================================
//  Load built-in theme QSS
// ============================================================
QString ThemeManager::loadBuiltinQss(ThemeMode mode) const
{
    QString path = (mode == ThemeMode::Light)
    ? QStringLiteral(":/themes/themes/light.qss")
    : QStringLiteral(":/themes/themes/dark.qss");

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LogRouter::instance().warning(
            QString("[ThemeManager] Cannot open theme file: %1").arg(path));
        return QString();
    }

    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    return ts.readAll();
}

// ============================================================
//  Load user-provided custom QSS
// ============================================================
QString ThemeManager::loadCustomQss(const QString &path) const
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LogRouter::instance().warning(
            QString("[ThemeManager] Cannot open custom QSS: %1").arg(path));
        return QString();
    }

    QTextStream ts(&f);
    ts.setEncoding(QStringConverter::Utf8);
    return ts.readAll();
}

// ============================================================
//  Token Engine helpers (unchanged logic, no logging inside)
// ============================================================

QMap<QString, QString> ThemeManager::buildDefaultTokenMap(ThemeMode mode) const
{
    QMap<QString, QString> t;
    if (mode == ThemeMode::Light) {
        t["@bg1"]="#F3F4F6"; t["@bg2"]="#E5E7EB"; t["@panel"]="#FFFFFF";
        t["@text"]="#111827"; t["@textMuted"]="#6B7280";
        t["@primary"]="#2563EB"; t["@accent"]="#3B82F6";
        t["@danger"]="#DC2626"; t["@border"]="#D1D5DB";
        t["@radius"]="6px"; t["@padding"]="4px";
    } else {
        t["@bg1"]="#111827"; t["@bg2"]="#1F2937"; t["@panel"]="#1F2933";
        t["@text"]="#E5E7EB"; t["@textMuted"]="#9CA3AF";
        t["@primary"]="#3B82F6"; t["@accent"]="#60A5FA";
        t["@danger"]="#F87171"; t["@border"]="#374151";
        t["@radius"]="6px"; t["@padding"]="4px";
    }
    return t;
}

QString ThemeManager::stripTokenDefinitionsAndOverrideMap(
    const QString &baseQss,
    QMap<QString, QString> &tokenMap) const
{
    QStringList out;
    for (const QString &raw : baseQss.split('\n')) {
        QString line = raw.trimmed();
        if (line.startsWith('@')) {
            int c = line.indexOf(':');
            if (c > 1) {
                QString key = line.left(c).trimmed();
                QString val = line.mid(c + 1).trimmed();
                if (val.endsWith(';')) val.chop(1);
                if (!key.isEmpty() && !val.isEmpty())
                    tokenMap[key] = val;
            }
            continue;
        }
        out.append(raw);
    }
    return out.join('\n');
}

QString ThemeManager::applyTokensToQss(
    const QString &qss,
    const QMap<QString, QString> &tokenMap) const
{
    QString result = qss;
    for (auto it = tokenMap.begin(); it != tokenMap.end(); ++it)
        result.replace(it.key(), it.value());
    return result;
}

QString ThemeManager::buildFinalStylesheet(
    ThemeMode mode,
    const QString &baseQss) const
{
    QMap<QString, QString> tokens = buildDefaultTokenMap(mode);
    QString noDefs = stripTokenDefinitionsAndOverrideMap(baseQss, tokens);
    QString themed = applyTokensToQss(noDefs, tokens);

    QString fontQss = QString(
                          "QWidget{font-family:\"%1\";font-size:%2pt;}"
                          "QPlainTextEdit,QTextEdit,QLineEdit{font-size:%3pt;}")
                          .arg(m_appFont.family())
                          .arg(m_appFont.pointSize())
                          .arg(m_logFont.pointSize());

    return themed + "\n" + fontQss;
}
