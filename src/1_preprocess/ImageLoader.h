// ============================================================
//  OCRtoODT — Preprocess: Image Loader
//  File: 1_preprocess/imageloader.h
//
//  Responsibility:
//      Load image files from disk while applying EXIF-based
//      orientation (auto-rotate). This provides a clean,
//      unified QImage for further preprocessing.
// ============================================================

#ifndef PREPROCESS_IMAGELOADER_H
#define PREPROCESS_IMAGELOADER_H

#include <QImage>
#include <QString>

namespace Ocr {
namespace Preprocess {

class ImageLoader
{
public:
    // --------------------------------------------------------
    // Load an image from disk using QImageReader with
    // autoTransform enabled (EXIF orientation).
    //
    //  - path         : absolute or relative path to the image
    //  - errorMessage : optional; receives human-readable
    //                   error description on failure
    //
    // Returns:
    //      QImage (null on failure).
    // --------------------------------------------------------
    static QImage loadWithExif(const QString &path,
                               QString *errorMessage = nullptr);
};

} // namespace Preprocess
} // namespace Ocr

#endif // PREPROCESS_IMAGELOADER_H
