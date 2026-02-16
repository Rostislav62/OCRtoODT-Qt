// ============================================================
//  OCRtoODT — Theme Manager 2.0 (+ Token Engine)
//  File: src/core/ThemeManager.h
//
//  Responsibility:
//      Centralized theme engine for the whole application.
//      Handles:
//          - theme mode: light / dark / auto / system / custom
//          - base QSS (built-in light.qss / dark.qss)
//          - optional user QSS file
//          - application fonts (UI + log font)
//          - toolbar style (icons / text / icons + text)
//          - thumbnail size
//          - THEME TOKENS (@primary, @bg1, @accent, etc.)
//          - UI language via QTranslator (ui.language)
//
// ============================================================

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QFont>
#include <QColor>
#include <QMap>
#include <QString>
#include <QTranslator>

class ThemeManager : public QObject
{
    Q_OBJECT

public:
    enum class ThemeMode {
        Light,
        Dark,
        Auto,
        System,
        Custom
    };
    Q_ENUM(ThemeMode)

    static ThemeManager* instance();

    void applyAllFromConfig();

    ThemeMode currentMode()      const { return m_mode; }
    QFont     appFont()          const { return m_appFont; }
    QFont     logFont()          const { return m_logFont; }
    QString   toolbarStyle()     const { return m_toolbarStyle; }
    int       thumbnailSize()    const { return m_thumbnailSize; }
    QString   activeStylesheet() const { return m_activeStylesheet; }

public slots:
    void reloadFromSettings();

signals:
    void themeApplied(ThemeManager::ThemeMode mode);
    void fontsApplied(const QFont &appFont, const QFont &logFont);
    void toolbarStyleChanged(const QString &style);
    void thumbnailSizeChanged(int size);

private:
    explicit ThemeManager(QObject *parent = nullptr);
    Q_DISABLE_COPY(ThemeManager)

    // Core steps
    void loadStateFromConfig();
    void applyThemeToApplication();
    void applyFontsToApplication();

    ThemeMode resolveMode(const QString &modeString,
                          const QString &customQssPath) const;
    ThemeMode resolveAutoMode() const;

    QString   loadBuiltinQss(ThemeMode mode) const;
    QString   loadCustomQss(const QString &path) const;

    // FINAL QSS builder: base QSS + tokens + font overlay
    QString   buildFinalStylesheet(ThemeMode mode,
                                 const QString &baseQss) const;

    // ================= Token Engine helpers =================
    QMap<QString, QString> buildDefaultTokenMap(ThemeMode mode) const;

    QString stripTokenDefinitionsAndOverrideMap(
        const QString &baseQss,
        QMap<QString, QString> &tokenMap) const;

    QString applyTokensToQss(const QString &qss,
                             const QMap<QString, QString> &tokenMap) const;


    ThemeMode m_mode = ThemeMode::Dark;

    // IMPORTANT: used by ThemeManager.cpp (custom theme)
    QString   m_customQssPath;

    // UI styling state
    QFont     m_appFont;
    QFont     m_logFont;
    QString   m_toolbarStyle;
    int       m_thumbnailSize = 160;
    QString   m_activeStylesheet;

};

#endif // THEMEMANAGER_H
