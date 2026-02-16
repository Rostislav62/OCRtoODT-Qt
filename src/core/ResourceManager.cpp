// ============================================================
//  OCRtoODT — Resource Manager
//  File: resourcemanager.cpp
//
//  See resourcemanager.h for high-level description.
//
//  Changes in STEP 6.1.5.1:
//      ✔ Removed direct qDebug() usage
//      ✔ Integrated centralized LogRouter
//      ✔ Preserved all existing behavior and logic
//      ✔ Logging is now filtered by logging.level
// ============================================================

#include "ResourceManager.h"

#include "ConfigManager.h"
#include "core/LogRouter.h"
#include "../systeminfo/systeminfo.h"

#include <QVariant>

namespace Core {

// ============================================================
//  Singleton access
// ============================================================
ResourceManager& ResourceManager::instance()
{
    static ResourceManager rm;
    return rm;
}

// ============================================================
//  Constructor
// ============================================================
ResourceManager::ResourceManager()
{
    refresh();
}

// ============================================================
//  Public API: refresh
// ============================================================
void ResourceManager::refresh()
{
    computeFromConfig();

    // STEP 6.1.5.1:
    // Replaced qDebug() with LogRouter::info()
    // This message is informational and should respect logging.level >= 3
    LogRouter::instance().info(
        QString("[ResourceManager] Configured threads: "
                "pdfThumb=%1, pdfPage=%2, imgThumb=%3 (auto mode=%4)")
            .arg(m_pdfThumbnailThreads)
            .arg(m_pdfPageThreads)
            .arg(m_imageThumbThreads)
            .arg(m_autoMode ? "true" : "false")
        );
}

// ============================================================
//  Getters
// ============================================================
int ResourceManager::pdfThumbnailThreads() const
{
    return m_pdfThumbnailThreads;
}

int ResourceManager::pdfPageThreads() const
{
    return m_pdfPageThreads;
}

int ResourceManager::imageThumbnailThreads() const
{
    return m_imageThumbThreads;
}

// ------------------------------------------------------------
//  Debug summary (string only, no logging side effects)
// ------------------------------------------------------------
QString ResourceManager::summary() const
{
    QString mode = m_autoMode ? "auto" : "manual";
    QString cpu  = QString::fromUtf8(si_cpu_brand_string());
    long long ramMB = si_total_ram_mb();

    return QStringLiteral(
               "ResourceManager summary:\n"
               "  Mode: %1\n"
               "  CPU:  %2\n"
               "  Logical threads: %3\n"
               "  Total RAM: %4 MB\n"
               "  pdfThumbnailThreads: %5\n"
               "  pdfPageThreads:      %6\n"
               "  imageThumbThreads:   %7\n")
        .arg(mode)
        .arg(cpu)
        .arg(m_logicalThreads)
        .arg(ramMB)
        .arg(m_pdfThumbnailThreads)
        .arg(m_pdfPageThreads)
        .arg(m_imageThumbThreads);
}

// ============================================================
//  Internal: computeFromConfig
// ============================================================
void ResourceManager::computeFromConfig()
{
    ConfigManager &cfg = ConfigManager::instance();

    // Read logical thread count
    int lt = si_cpu_logical_threads();
    if (lt <= 0)
        lt = 4;

    m_logicalThreads = lt;

    // Auto mode?
    m_autoMode = autoModeEnabled();

    if (m_autoMode)
    {
        int pdfThumb = lt / 4;
        int pdfPage  = lt / 2;
        int imgThumb = lt / 2;

        if (pdfThumb < 1) pdfThumb = 1;
        if (pdfPage  < 1) pdfPage  = 1;
        if (imgThumb < 1) imgThumb = 1;

        m_pdfThumbnailThreads = pdfThumb;
        m_pdfPageThreads      = pdfPage;
        m_imageThumbThreads   = imgThumb;
    }
    else
    {
        int pdfThumb = readInt("threading.pdf_thumbnail_threads", 2);
        int pdfPage  = readInt("threading.pdf_page_threads", 4);
        int imgThumb = readInt("threading.image_thumbnail_threads", 4);

        auto clamp = [lt](int v){
            if (v < 1)  v = 1;
            if (v > lt) v = lt;
            return v;
        };

        m_pdfThumbnailThreads = clamp(pdfThumb);
        m_pdfPageThreads      = clamp(pdfPage);
        m_imageThumbThreads   = clamp(imgThumb);
    }
}

// ============================================================
//  Helpers
// ============================================================
bool ResourceManager::autoModeEnabled() const
{
    const ConfigManager &cfg = ConfigManager::instance();
    QString s = cfg.get("threading.auto", "false")
                    .toString()
                    .trimmed()
                    .toLower();

    if (s == "true" || s == "yes")
        return true;

    bool ok = false;
    int iv = s.toInt(&ok);
    return (ok && iv != 0);
}

int ResourceManager::readInt(const QString &path, int defaultValue) const
{
    const ConfigManager &cfg = ConfigManager::instance();
    QVariant v = cfg.get(path, defaultValue);

    bool ok = false;
    int result = v.toString().trimmed().toInt(&ok);
    return ok ? result : defaultValue;
}

} // namespace Core
