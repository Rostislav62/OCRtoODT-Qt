// ============================================================
//  OCRtoODT — OCR Page Worker
//  File: src/2_ocr/OcrPageWorker.cpp
//
//  Responsibility:
//      Execute OCR for ONE page using prepared preprocessing output.
//
//  IMPORTANT:
//      • Multipass TSV is produced in RAM (no per-pass temp files).
//      • This worker NEVER decides RAM vs DISK for IMAGE source.
//      • It MUST strictly follow PageJob contract.
//
//  Transitional note (this step):
//      • Worker still writes ONE final TSV to disk to keep the rest
//        of the pipeline working unchanged.
//      • Next step will move this final TSV handoff to RAM contract
//        at OcrPipelineWorker level.
// ============================================================

#include "2_ocr/OcrPageWorker.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QCoreApplication>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include <tesseract/baseapi.h>

#include "core/ConfigManager.h"
#include "core/LogRouter.h"

#include "2_ocr/OcrPassConfig.h"
#include "2_ocr/OcrTsvQuality.h"
#include "2_ocr/OcrMultipassSelector.h"

using namespace Ocr;

// ============================================================
// Build FINAL TSV path (always under cache/)
// ============================================================
QString OcrPageWorker::buildTsvPath(int globalIndex)
{
    ConfigManager &cfg = ConfigManager::instance();

    const QString base =
        cfg.get("general.ocr_path", "cache/ocr").toString();

    QDir dir(base + "/tsv");
    QDir().mkpath(dir.absolutePath());

    return dir.filePath(
        QString("page_%1.tsv")
            .arg(globalIndex, 4, 10, QLatin1Char('0')));
}

// ============================================================
// Helper: sanitize TSV (decimal comma → dot in conf column)
// ============================================================
static QString sanitizeTsvConf(const QString &tsv)
{
    if (tsv.isEmpty())
        return tsv;

    QString out;
    out.reserve(tsv.size());

    const QStringList lines =
        tsv.split('\n', Qt::KeepEmptyParts);

    for (const QString &ln : lines)
    {
        if (ln.isEmpty())
        {
            out += '\n';
            continue;
        }

        QStringList cols =
            ln.split('\t', Qt::KeepEmptyParts);

        // Column 10 is confidence
        if (cols.size() >= 11 && cols[10].contains(','))
            cols[10].replace(',', '.');

        out += cols.join('\t');
        out += '\n';
    }

    return out;
}

// ============================================================
// Perform OCR for ONE page (wrapper without cancel)
// ============================================================
OcrPageResult OcrPageWorker::run(const Ocr::Preprocess::PageJob &job)
{
    return run(job, nullptr);
}

// ============================================================
// Perform OCR for ONE page (cooperative cancel version)
// ============================================================
OcrPageResult OcrPageWorker::run(const Ocr::Preprocess::PageJob &job,
                                 const std::atomic_bool *cancelFlag)
{
    // --------------------------------------------------------
    // Entry log — confirms contract inputs
    // --------------------------------------------------------
    LogRouter::instance().info(
        QString("[OcrPageWorker] START page=%1 keepInRam=%2 enhancedMat=%3 enhancedPath='%4' ocrDpi=%5")
            .arg(job.globalIndex)
            .arg(job.keepInRam ? "true" : "false")
            .arg(job.enhancedMat.empty() ? "EMPTY" : "OK")
            .arg(job.enhancedPath)
            .arg(job.ocrDpi));

    OcrPageResult result;
    result.globalIndex = job.globalIndex;

    // --------------------------------------------------------
    // Early cancel check (before any heavy work)
    // --------------------------------------------------------
    if (cancelFlag && cancelFlag->load())
    {
        LogRouter::instance().info(
            QString("[OcrPageWorker] CANCELLED before processing page=%1")
                .arg(job.globalIndex));
        return result;
    }

    // =========================================================
    // 1) INPUT IMAGE (CONTRACT-DRIVEN)
    // =========================================================
    cv::Mat gray;

    if (job.keepInRam)
    {
        gray = job.enhancedMat;

        LogRouter::instance().info(
            QString("[OcrPageWorker] Page %1: using enhancedMat (RAM)")
                .arg(job.globalIndex));
    }
    else
    {
        if (job.enhancedPath.trimmed().isEmpty())
        {
            LogRouter::instance().error(
                QString("[OcrPageWorker] Page %1: enhancedPath EMPTY (contract violation)")
                    .arg(job.globalIndex));

            result.errorMessage =
                QString("enhancedPath empty for page %1").arg(job.globalIndex);
            return result;
        }

        // --------------------------------------------------------
        // Cancel check BEFORE disk read (heavy I/O)
        // --------------------------------------------------------
        if (cancelFlag && cancelFlag->load())
        {
            LogRouter::instance().info(
                QString("[OcrPageWorker] CANCELLED before disk load page=%1")
                    .arg(job.globalIndex));
            return result;
        }

        gray = cv::imread(job.enhancedPath.toStdString(),
                          cv::IMREAD_GRAYSCALE);

        LogRouter::instance().info(
            QString("[OcrPageWorker] Page %1: using enhancedPath (DISK) '%2'")
                .arg(job.globalIndex)
                .arg(job.enhancedPath));
    }

    if (gray.empty() || gray.type() != CV_8UC1)
    {
        LogRouter::instance().error(
            QString("[OcrPageWorker] Page %1: invalid Gray8 input")
                .arg(job.globalIndex));

        result.errorMessage =
            QString("Invalid Gray8 input for page %1").arg(job.globalIndex);
        return result;
    }

    // --------------------------------------------------------
    // Cancel check after image acquisition
    // --------------------------------------------------------
    if (cancelFlag && cancelFlag->load())
    {
        LogRouter::instance().info(
            QString("[OcrPageWorker] CANCELLED after image load page=%1")
                .arg(job.globalIndex));
        OcrPageResult r;
        r.globalIndex = job.globalIndex;
        r.success = false;
        r.tsvText.clear();
        return r;
    }

    // =========================================================
    // 2) OCR CONFIGURATION
    // =========================================================
    ConfigManager &cfg = ConfigManager::instance();

    const QString languages =
        cfg.get("ocr.languages", "eng")
            .toString()
            .split(',', Qt::SkipEmptyParts)
            .join('+');

    const int oem = cfg.get("ocr.tesseract_oem", 1).toInt();
    const int dpi = job.ocrDpi;

    // ---------------------------------------------------------
    // Controlled multipass configuration
    // ---------------------------------------------------------
    QList<int> psmList;

    for (int i = 1; ; ++i)
    {
        const QString key = QString("ocr.psm_%1").arg(i);
        const QVariant v = cfg.get(key);

        if (!v.isValid())
            break;

        bool ok = false;
        const int psm = v.toInt(&ok);

        if (!ok || psm < 0 || psm > 13)
            continue;

        psmList << psm;
    }

    if (psmList.isEmpty())
        psmList << 4;

    QList<OcrPassResult> passResults;

    // =========================================================
    // 3) MULTI-PASS OCR LOOP (HEAVY SECTION)
    // =========================================================
    for (int psm : psmList)
    {
        // Cooperative cancel before each pass
        if (cancelFlag && cancelFlag->load())
        {
            LogRouter::instance().info(
                QString("[OcrPageWorker] CANCELLED during multipass page=%1")
                    .arg(job.globalIndex));
            return result;
        }

        OcrPassResult pass;
        pass.config.passName  = QString("psm%1").arg(psm);
        pass.config.languages = languages;
        pass.config.psm       = psm;
        pass.config.oem       = oem;
        pass.config.dpi       = dpi;

        // --------------------------------------------------------
        // Cancel check BEFORE TessBaseAPI init (heavy)
        // --------------------------------------------------------
        if (cancelFlag && cancelFlag->load())
        {
            LogRouter::instance().info(
                QString("[OcrPageWorker] CANCELLED before Tess init page=%1")
                    .arg(job.globalIndex));
            return result;
        }

        tesseract::TessBaseAPI api;

        // --------------------------------------------------------
        // Cancel check BEFORE api.Init
        // --------------------------------------------------------
        if (cancelFlag && cancelFlag->load())
        {
            LogRouter::instance().info(
                QString("[OcrPageWorker] CANCELLED before api.Init page=%1")
                    .arg(job.globalIndex));
            return result;
        }

        if (api.Init(nullptr,
                     languages.toUtf8().constData(),
                     static_cast<tesseract::OcrEngineMode>(oem)) != 0)
        {
            continue;
        }

        api.SetPageSegMode(
            static_cast<tesseract::PageSegMode>(psm));

        api.SetVariable("user_defined_dpi",
                        QByteArray::number(dpi).constData());

        // --------------------------------------------------------
        // Cancel check BEFORE SetImage
        // --------------------------------------------------------
        if (cancelFlag && cancelFlag->load())
        {
            LogRouter::instance().info(
                QString("[OcrPageWorker] CANCELLED before SetImage page=%1")
                    .arg(job.globalIndex));
            return result;
        }

        api.SetImage(gray.data,
                     gray.cols,
                     gray.rows,
                     1,
                     static_cast<int>(gray.step));

        // --------------------------------------------------------
        // Cancel check BEFORE GetTSVText (very heavy call)
        // --------------------------------------------------------
        if (cancelFlag && cancelFlag->load())
        {
            LogRouter::instance().info(
                QString("[OcrPageWorker] CANCELLED before GetTSVText page=%1")
                    .arg(job.globalIndex));
            return result;
        }

        char *raw = api.GetTSVText(0);

        if (!raw)
            continue;

        const QString tsvRaw = QString::fromUtf8(raw);
        delete [] raw;

        pass.tsvText = sanitizeTsvConf(tsvRaw);

        // --------------------------------------------------------
        // Cancel check BEFORE quality analysis
        // --------------------------------------------------------
        if (cancelFlag && cancelFlag->load())
        {
            LogRouter::instance().info(
                QString("[OcrPageWorker] CANCELLED before quality analysis page=%1")
                    .arg(job.globalIndex));
            return result;
        }

        pass.quality = analyzeTsvQualityFromText(pass.tsvText);

        passResults << pass;
    }

    if (passResults.isEmpty())
    {
        result.errorMessage =
            QString("OCR failed for page %1").arg(job.globalIndex);
        return result;
    }

    // =========================================================
    // 4) SELECT BEST PASS
    // =========================================================
    const OcrPassResult best =
        selectBestOcrPass(passResults);

    // =========================================================
    // 5) FINAL RESULT
    // =========================================================
    result.success = true;
    result.tsvText = best.tsvText;

    LogRouter::instance().info(
        QString("[OcrPageWorker] SUCCESS page=%1 best=%2 score=%3")
            .arg(job.globalIndex)
            .arg(best.config.passName)
            .arg(best.quality.score));

    return result;
}
