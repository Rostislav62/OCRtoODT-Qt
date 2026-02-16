// ============================================================
//  OCRtoODT — OCR PDF Page Cache
//  File: core/ocrpdfcache.cpp
// ============================================================

#include "core/ocrPdfCache.h"

OcrPdfCache &OcrPdfCache::instance()
{
    static OcrPdfCache inst;
    return inst;
}

OcrPdfCache::OcrPdfCache(QObject *parent)
    : QObject(parent)
{
}

void OcrPdfCache::clear()
{
    QMutexLocker lock(&m_mutex);
    m_cache.clear();
}
