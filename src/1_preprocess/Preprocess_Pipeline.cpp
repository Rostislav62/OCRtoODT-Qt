// ============================================================
//  OCRtoODT — Preprocess Pipeline (Policy-Driven, Parallel Engine)
//  File: src/1_preprocess/Preprocess_Pipeline.cpp
//
//  Responsibility:
//      Parallel preprocessing engine for document pages.
//
//  Behavior (FINAL):
//      - Process pages in parallel
//      - Always produce enhanced image in RAM first
//      - Disk output is controlled STRICTLY by general.mode
//        and general.debug_mode
//
// ============================================================

// ============================================================
//  OCRtoODT — Preprocess Pipeline
//  File: src/1_preprocess/Preprocess_Pipeline.cpp
// ============================================================

#include "1_preprocess/Preprocess_Pipeline.h"

#include <QtConcurrent>
#include <QThreadPool>
#include <QDir>
#include <QImage>

#include "core/ConfigManager.h"
#include "core/LogRouter.h"

#include "1_preprocess/ImageLoader.h"
#include "1_preprocess/ImageAnalyzer.h"
#include "1_preprocess/StrategySelector.h"

using namespace Ocr::Preprocess;

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------
static QImage grayMatToQImage(const cv::Mat &mat)
{
    if (mat.empty() || mat.type() != CV_8UC1)
        return QImage();

    QImage img(mat.cols, mat.rows, QImage::Format_Grayscale8);
    for (int y = 0; y < mat.rows; ++y)
        memcpy(img.scanLine(y), mat.ptr(y),
               static_cast<size_t>(mat.cols));
    return img;
}

static QString buildEnhancedPath(int globalIndex,
                                 const QString &logicalBaseDir)
{
    const QString baseDir = QStringLiteral("cache/") + logicalBaseDir;
    QDir().mkpath(baseDir);

    return QDir(baseDir).filePath(
        QString("page_%1.png")
            .arg(globalIndex, 4, 10, QLatin1Char('0')));
}

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
PreprocessPipeline::PreprocessPipeline(QObject *parent)
    : QObject(parent)
{
    configureThreadPool();
}

// ------------------------------------------------------------
// Thread pool config
// ------------------------------------------------------------
void PreprocessPipeline::configureThreadPool()
{
    ConfigManager &cfg = ConfigManager::instance();

    const bool parallel =
        cfg.get("general.parallel_enabled", true).toBool();

    const QVariant num =
        cfg.get("general.num_processes", "auto");

    m_threadCount =
        (!parallel) ? 1 :
            (num.toString() == "auto"
                 ? QThread::idealThreadCount()
                 : std::max(1, num.toInt()));

    QThreadPool::globalInstance()->setMaxThreadCount(m_threadCount);

    LogRouter::instance().info(
        QString("[PreprocessPipeline] Using %1 threads")
            .arg(m_threadCount));
}

// ------------------------------------------------------------
// Main API
// ------------------------------------------------------------
QVector<PageJob> PreprocessPipeline::run(
    const QVector<Core::VirtualPage> &pages)
{
    QVector<PageJob> results(pages.size());
    if (pages.isEmpty())
        return results;

    ConfigManager &cfg = ConfigManager::instance();

    const QString mode =
        cfg.get("general.mode", "ram_only").toString();

    const bool debugMode =
        cfg.get("general.debug_mode", false).toBool();

    const bool diskOnly = (mode == "disk_only");

    const QString preprocessPath =
        cfg.get("general.preprocess_path", "preprocess").toString();

    const QString profile =
        cfg.get("preprocess.profile", "scanner").toString();

    auto lambda =
        [=](const Core::VirtualPage &vp) -> PageJob
    {
        PageJob job;

        job =
            m_processor.processSingleWithProfile(
                vp, vp.getGlobalIndex(), profile);

        // ----------------------------------------------------
        // IMAGE ANALYSIS (READ-ONLY)
        // ----------------------------------------------------
        ImageDiagnostics diag =
            ImageAnalyzer::analyzeGray(job.enhancedMat);

        job.ocrDpi = diag.suggestedOcrDpi;

        LogRouter::instance().info(
            QString("[PreprocessPipeline] Page %1 OCR DPI=%2")
                .arg(job.globalIndex)
                .arg(job.ocrDpi));

        // ----------------------------------------------------
        // Disk policy
        // ----------------------------------------------------
        if ((diskOnly || debugMode) && !job.enhancedMat.empty())
        {
            const QString outPath =
                buildEnhancedPath(job.globalIndex, preprocessPath);

            QImage img = grayMatToQImage(job.enhancedMat);
            if (!img.isNull() && img.save(outPath)) {
                job.enhancedPath = outPath;
                job.savedToDisk  = true;
            }
        }

        job.keepInRam = !diskOnly;
        if (diskOnly)
            job.enhancedMat.release();

        return job;
    };

    QFuture<PageJob> future =
        QtConcurrent::mapped(pages, lambda);

    future.waitForFinished();

    for (int i = 0; i < pages.size(); ++i)
        results[i] = future.resultAt(i);

    return results;
}
