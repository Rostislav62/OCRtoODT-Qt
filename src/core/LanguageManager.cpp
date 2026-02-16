#include "LanguageManager.h"

#include "core/ConfigManager.h"
#include "core/LogRouter.h"

#include <QCoreApplication>
#include <QLocale>
#include <QDir>
#include <QFileInfo>

// ------------------------------------------------------------
// Singleton
// ------------------------------------------------------------
LanguageManager *LanguageManager::instance()
{
    static LanguageManager *inst = new LanguageManager(qApp);
    return inst;
}

LanguageManager::LanguageManager(QObject *parent)
    : QObject(parent)
{
}

// ------------------------------------------------------------
// Initialization (called once from main.cpp)
// ------------------------------------------------------------
void LanguageManager::initialize()
{
    ConfigManager &cfg = ConfigManager::instance();

    QString lang = cfg.get("ui.language", "").toString().trimmed();
    if (lang.isEmpty())
        lang = detectInitialLanguage();

    if (!setLanguage(lang))
        setLanguage("en"); // hard fallback
}

// ------------------------------------------------------------
// Apply language at runtime (NO restart)
// ------------------------------------------------------------
bool LanguageManager::setLanguage(const QString &langCode)
{
    const QString code = langCode.trimmed().isEmpty() ? "en" : langCode.trimmed();

    QCoreApplication::removeTranslator(&m_translator);

    const QString qmPath = resolveQmPath(code);
    if (qmPath.isEmpty())
    {
        LogRouter::instance().warning(
            QString("[LanguageManager] No .qm found for language: %1").arg(code));
        return false;
    }

    LogRouter::instance().info(
        QString("[LanguageManager] Using qm: %1 (exists=%2)")
            .arg(qmPath)
            .arg(QFile::exists(qmPath) ? "true" : "false"));


    if (!m_translator.load(qmPath))
        return false;

    QCoreApplication::installTranslator(&m_translator);
    m_currentLanguage = code;

    // persist choice
    ConfigManager::instance().set("ui.language", code);
    ConfigManager::instance().save();

    emit languageChanged(code);

    LogRouter::instance().info(
        QString("[LanguageManager] Language switched to: %1").arg(code));

    return true;
}

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------
QString LanguageManager::currentLanguage() const
{
    return m_currentLanguage;
}

QStringList LanguageManager::availableLanguages() const
{
    // minimal now; can be extended later by scanning directory
    return { "en", "ru", "ro" };
}

QString LanguageManager::detectInitialLanguage() const
{
    const QString locale = QLocale::system().name(); // e.g. ro_RO
    const QString shortCode = locale.left(2);

    if (availableLanguages().contains(shortCode))
        return shortCode;

    return "en";
}
// ------------------------------------------------------------
// Resolve .qm path (QRC first, then disk fallback)
// ------------------------------------------------------------
QString LanguageManager::resolveQmPath(const QString &langCode) const
{
    // 1) Prefer embedded resources (most reliable in build/run)
    //    Expected qrc prefix: /translations
    //    Example: :/translations/ocrtoodt_ru.qm
    const QString qrcPath = QString(":/translations/ocrtoodt_%1.qm").arg(langCode);
    if (QFile::exists(qrcPath))
        return qrcPath;

    // 2) Disk fallback: project layout (run from build dir)
    const QString fileName = QString("ocrtoodt_%1.qm").arg(langCode);

    const QStringList anchors = {
        QCoreApplication::applicationDirPath(),
        QDir::currentPath()
    };

    for (const QString &anchor : anchors)
    {
        QDir d(anchor);
        for (int up = 0; up < 8; ++up)
        {
            const QString candidate =
                d.absoluteFilePath("resources/translations/" + fileName);

            if (QFileInfo::exists(candidate))
                return candidate;

            // 3) Extra fallback: sometimes .qm is generated directly into build folder root
            const QString buildCandidate = d.absoluteFilePath(fileName);
            if (QFileInfo::exists(buildCandidate))
                return buildCandidate;

            if (!d.cdUp())
                break;
        }
    }

    return QString();
}
