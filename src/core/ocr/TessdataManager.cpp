#include "TessdataManager.h"

#include <QDir>
#include <QStandardPaths>
#include <QFileInfoList>

TessdataManager::TessdataManager()
{
    const QString base =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);

    m_dir = base + "/tessdata";

    QDir().mkpath(m_dir);
}

QString TessdataManager::tessdataDir() const
{
    return m_dir;
}

QString TessdataManager::tessdataParentDir() const
{
    QDir dir(m_dir);
    dir.cdUp();
    return dir.absolutePath();
}

QString TessdataManager::traineddataPath(const QString& code) const
{
    return m_dir + "/" + code + ".traineddata";
}

bool TessdataManager::hasLanguage(const QString& code) const
{
    return QFile::exists(traineddataPath(code));
}

QStringList TessdataManager::installedLanguages() const
{
    QDir dir(m_dir);

    QStringList result;

    QFileInfoList files =
        dir.entryInfoList(QStringList() << "*.traineddata", QDir::Files);

    for (auto& f : files)
        result << f.baseName();

    return result;
}
