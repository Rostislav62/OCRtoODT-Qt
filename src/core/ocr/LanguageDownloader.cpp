#include "LanguageDownloader.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>

static const QString BASE_URL =
    "https://github.com/tesseract-ocr/tessdata_best/raw/main/";

LanguageDownloader::LanguageDownloader(QObject* parent)
    : QObject(parent)
{
}

void LanguageDownloader::downloadLanguage(
    const QString& code,
    const QString& destPath)
{
    const QString url = BASE_URL + code + ".traineddata";

    QNetworkReply* reply =
        m_net.get(QNetworkRequest(QUrl(url)));

    connect(reply, &QNetworkReply::downloadProgress,
            this, &LanguageDownloader::downloadProgress);

    connect(reply, &QNetworkReply::finished, [this, reply, code, destPath]()
            {
                if (reply->error() != QNetworkReply::NoError)
                {
                    emit downloadError(code, reply->errorString());
                    reply->deleteLater();
                    return;
                }

                QFile file(destPath);
                file.open(QIODevice::WriteOnly);
                file.write(reply->readAll());
                file.close();

                emit downloadFinished(code);

                reply->deleteLater();
            });
}
