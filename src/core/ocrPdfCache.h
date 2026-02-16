// ============================================================
//  OCRtoODT — OCR PDF Page Cache
//  File: core/ocrpdfcache.h
//
//  Responsibility:
//      Cache rendered PDF pages (QImage) for OCR pipeline.
//      Key = (pdfPath, pageIndex, dpi).
//
//      This avoids re-rendering the same page in different
//      stages: normalize, enhancement, OCR preview, etc.
// ============================================================

#ifndef OCRPDFCACHE_H
#define OCRPDFCACHE_H

#include <QObject>
#include <QHash>
#include <QImage>
#include <QMutex>

class OcrPdfCache : public QObject
{
    Q_OBJECT

public:
    static OcrPdfCache &instance();

    // --------------------------------------------------------
    // Get cached image if available; otherwise call 'renderer'
    // to render QImage, store it in cache and return it.
    //
    //  - renderer must be a callable: QImage()
    // --------------------------------------------------------
    template <typename Renderer>
    QImage getOrRender(const QString &pdfPath,
                       int pageIndex,
                       int dpi,
                       Renderer renderer)
    {
        const QString key =
            QString("%1|%2|%3").arg(pdfPath).arg(pageIndex).arg(dpi);

        {
            QMutexLocker lock(&m_mutex);
            auto it = m_cache.find(key);
            if (it != m_cache.end())
                return it.value();
        }

        // Not cached → render
        QImage img = renderer();
        if (!img.isNull())
        {
            QMutexLocker lock(&m_mutex);
            m_cache.insert(key, img);
        }
        return img;
    }

    // Clear all cached pages (e.g. at the end of OCR run)
    void clear();

private:
    explicit OcrPdfCache(QObject *parent = nullptr);
    Q_DISABLE_COPY(OcrPdfCache)

    QMutex                      m_mutex;
    QHash<QString, QImage>      m_cache;
};

#endif // OCRPDFCACHE_H
