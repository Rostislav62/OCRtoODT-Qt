// ============================================================
//  OCRtoODT — Image Thumbnail Provider
//  File: 0_input/imagethumbnailprovider.h
//
//  Responsibility:
//      Asynchronously generate QPixmap thumbnails for the file list.
//
//      STEP 2 addition:
//          Support thumbnails from RAM (QImage) in addition to
//          loading from disk. The API still exposes a generic
//          string key that DocumentController can match.
// ============================================================

#ifndef IMAGETHUMBNAILPROVIDER_H
#define IMAGETHUMBNAILPROVIDER_H

#include <QObject>
#include <QPixmap>
#include <QImage>
#include <QSize>

namespace Input {

class ImageThumbnailProvider : public QObject
{
    Q_OBJECT

public:
    explicit ImageThumbnailProvider(QObject *parent = nullptr);

    // ------------------------------------------------------------
    // Disk path → QPixmap thumbnail (async)
    // ------------------------------------------------------------
    void requestThumbnail(const QString &imagePath,
                          const QSize   &maxSize);

    // ------------------------------------------------------------
    // RAM image → QPixmap thumbnail (async)
    //
    // key:
    //     Any string that DocumentController can later match.
    //     Example: "enhanced:3" or an absolute disk path.
    // ------------------------------------------------------------
    void requestThumbnailFromImage(const QString &key,
                                   const QImage  &image,
                                   const QSize   &maxSize);

signals:
    void thumbnailReady(const QString &key,
                        const QPixmap &pix);
};

} // namespace Input

#endif // IMAGETHUMBNAILPROVIDER_H
