// ============================================================
//  OCRtoODT — Preprocess: Image Loader
//  File: 1_preprocess/imageloader.cpp
// ============================================================

#include "ImageLoader.h"

#include <QImageReader>

namespace Ocr {
namespace Preprocess {

QImage ImageLoader::loadWithExif(const QString &path,
                                 QString *errorMessage)
{
    QImageReader reader(path);
    reader.setAutoTransform(true); // apply EXIF orientation if present

    QImage img = reader.read();
    if (img.isNull())
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("Failed to load image '%1': %2")
            .arg(path, reader.errorString());
        }
    }

    return img;
}

} // namespace Preprocess
} // namespace Ocr
