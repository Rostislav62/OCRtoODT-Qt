// ============================================================
//  OCRtoODT — Resource Manager
//  File: resourcemanager.h
//
//  Responsibility:
//      Project-specific helper that converts global hardware
//      information (from SystemInfo) + config.yaml settings
//      into concrete thread counts for different parts of the
//      OCR pipeline.
//
//      This class is *not* a global system utility. It is
//      tailored for OCRtoODT and knows about:
//
//          - threading.pdf_thumbnail_threads
//          - threading.pdf_page_threads
//          - threading.image_thumbnail_threads
//          - threading.auto
//
//      Behavior:
//          * If threading.auto == true
//              - read logical CPU threads from SystemInfo
//              - compute recommended values, e.g. for 32 threads:
//                    pdfThumbnail  = 32 / 4 = 8
//                    pdfPage       = 32 / 2 = 16
//                    imageThumb    = 32 / 2 = 16
//
//          * If threading.auto == false
//              - use explicit values from config.yaml,
//                clamped to [1 .. logicalThreads].
//
//      Usage:
//          ResourceManager& rm = ResourceManager::instance();
//          int pdfThumbThreads  = rm.pdfThumbnailThreads();
//          int pdfPageThreads   = rm.pdfPageThreads();
//          int imgThumbThreads  = rm.imageThumbnailThreads();
//
//      NOTE:
//          - This class does not know anything about Qt thread
//            pools directly; it only returns integers.
//          - PdfPageProvider / ImageThumbnailProvider will use
//            these values via setMaxThreadCount().
// ============================================================

#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <QString>

// Forward declaration (defined in core/configmanager.h)
class ConfigManager;

namespace Core {
// ============================================================
//  ResourceManager
// ============================================================
//
//  Singleton, lightweight. Reads config once on construction,
//  but can be refreshed manually if needed (e.g. after user
//  changed settings in the GUI and saved config.yaml).
// ============================================================
class ResourceManager
{
public:
    // --------------------------------------------------------
    // Singleton access
    // --------------------------------------------------------
    static ResourceManager& instance();

    // --------------------------------------------------------
    // Re-read configuration and recompute internal thread counts.
    //
    // Call this if threading.* values in config.yaml were changed
    // at runtime and ConfigManager::load/save were called again.
    // --------------------------------------------------------
    void refresh();

    // --------------------------------------------------------
    // Getters: thread counts for various subsystems
    // --------------------------------------------------------

    // Number of threads recommended for PDF thumbnail rendering
    // (PdfPageProvider → thumbnail pool).
    int pdfThumbnailThreads() const;

    // Number of threads recommended for full-page PDF rendering
    // (PdfPageProvider → page pool).
    int pdfPageThreads() const;

    // Number of threads recommended for image thumbnail rendering
    // (ImageThumbnailProvider → pool).
    int imageThumbnailThreads() const;

    // Debug helper: return a short human-readable summary
    // of current settings (auto/manual, thread counts, CPU info).
    QString summary() const;

private:
    ResourceManager();
    ResourceManager(const ResourceManager&) = delete;
    void operator=(const ResourceManager&) = delete;

    // Internal worker: read config + SystemInfo and compute
    // m_pdfThumbnailThreads / m_pdfPageThreads / m_imageThumbThreads.
    void computeFromConfig();

    // Helpers
    int  logicalThreads() const;
    bool autoModeEnabled() const;
    int  readInt(const QString &path, int defaultValue) const;

private:
    // Cached values: always >= 1
    int m_pdfThumbnailThreads = 2;
    int m_pdfPageThreads      = 4;
    int m_imageThumbThreads   = 4;

    // Cached last known logical thread count (for clamping)
    int m_logicalThreads      = 1;

    // Cached flag whether auto mode was used during last compute
    bool m_autoMode           = false;
};
}

#endif // RESOURCEMANAGER_H
