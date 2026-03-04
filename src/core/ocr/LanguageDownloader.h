#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>

class LanguageDownloader : public QObject
{
    Q_OBJECT

public:
    explicit LanguageDownloader(QObject* parent = nullptr);

    void downloadLanguage(const QString& code, const QString& destPath);

signals:
    void downloadProgress(qint64 received, qint64 total);
    void downloadFinished(const QString& code);
    void downloadError(const QString& code, const QString& error);

private:
    QNetworkAccessManager m_net;
};
