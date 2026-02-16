// ============================================================
//  OCRtoODT — Image Thumbnail Provider
//  File: 0_input/ImageThumbnailProvider.cpp
//
//  Responsibility:
//      Asynchronous thumbnail generation for UI file list.
//      Supports disk path and RAM image sources.
// ============================================================

#include "ImageThumbnailProvider.h"

#include <QtConcurrent>

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
Input::ImageThumbnailProvider::ImageThumbnailProvider(QObject *parent)
    : QObject(parent)
{
}

// ------------------------------------------------------------
// requestThumbnail (disk path)
// ------------------------------------------------------------
void Input::ImageThumbnailProvider::requestThumbnail(const QString &imagePath,
                                                     const QSize &maxSize)
{
    if (imagePath.isEmpty())
        return;

    QtConcurrent::run([this, imagePath, maxSize]()
                      {
                          QImage img(imagePath);
                          if (img.isNull())
                              return;

                          QImage scaled = img.scaled(
                              maxSize,
                              Qt::KeepAspectRatio,
                              Qt::SmoothTransformation);

                          emit thumbnailReady(imagePath, QPixmap::fromImage(scaled));
                      });
}

// ------------------------------------------------------------
// requestThumbnailFromImage (RAM image)
// ------------------------------------------------------------
void Input::ImageThumbnailProvider::requestThumbnailFromImage(
    const QString &key,
    const QImage &image,
    const QSize &maxSize)
{
    if (key.isEmpty() || image.isNull())
        return;

    // QImage is implicitly shared; passing by value is cheap.
    QImage imgCopy = image;

    QtConcurrent::run([this, key, imgCopy, maxSize]()
                      {
                          if (imgCopy.isNull())
                              return;

                          QImage scaled = imgCopy.scaled(
                              maxSize,
                              Qt::KeepAspectRatio,
                              Qt::SmoothTransformation);

                          emit thumbnailReady(key, QPixmap::fromImage(scaled));
                      });
}
