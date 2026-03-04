#pragma once

#include <QString>
#include <QStringList>

class TessdataManager
{
public:
    TessdataManager();

    // ~/.config/OCRtoODT/OCRtoODT/tessdata
    QString tessdataDir() const;

    // ~/.config/OCRtoODT/OCRtoODT
    QString tessdataParentDir() const;

    bool hasLanguage(const QString& code) const;

    QString traineddataPath(const QString& code) const;

    QStringList installedLanguages() const;

private:
    QString m_dir;
};
