// ============================================================
//  OCRtoODT — OCR Page Worker
//  File: src/2_ocr/OcrPageWorker.cpp
//
//  Responsibility:
//      Execute OCR for ONE page using prepared preprocessing output.
//
//  IMPORTANT:
//      • Multipass TSV is produced in RAM.
//      • Language is injected (RUN invariant) — no config reads for language.
//      • Uses cooperative cancel checks before heavy steps.
//
// ============================================================

#include "2_ocr/OcrPageWorker.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include <tesseract/baseapi.h>

#include "core/ConfigManager.h"
#include "core/LogRouter.h"
#include "core/ocr/OcrLanguageManager.h"

#include "2_ocr/OcrPassConfig.h"
#include "2_ocr/OcrTsvQuality.h"
#include "2_ocr/OcrMultipassSelector.h"

using namespace Ocr;

// ============================================================
// Helper: sanitize TSV (decimal comma → dot in confidence column)
// ============================================================
static QString sanitizeTsvConf(const QString &tsv)
{
    if (tsv.isEmpty())
        return tsv;

    QString out;
    out.reserve(tsv.size());

    const QStringList lines = tsv.split('\n', Qt::KeepEmptyParts);

    for (const QString &ln : lines)
    {
        if (ln.isEmpty())
        {
            out += '\n';
            continue;
        }

        QStringList cols = ln.split('\t', Qt::KeepEmptyParts);

        // Column 10 is confidence (Tesseract TSV format)
        if (cols.size() >= 11 && cols[10].contains(','))
            cols[10].replace(',', '.');

        out += cols.join('\t');
        out += '\n';
    }

    return out;
}

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
// Convenience wrapper: no cancel
// ============================================================
OcrPageResult OcrPageWorker::run(const Ocr::Preprocess::PageJob &job,
                                 const QString &languageString)
{
    return run(job, languageString, nullptr);
}

// ============================================================
// Perform OCR for ONE page (cooperative cancel version)
//
// languageString:
//   MUST be non-empty and already in Tesseract format: "eng+rus"
//
// NOTE:
//   This function does NOT decide which languages to use.
//   It only executes OCR with the injected language string.
// ============================================================
OcrPageResult OcrPageWorker::run(const Ocr::Preprocess::PageJob &job,
                                 const QString &languageString,
                                 const std::atomic_bool *cancelFlag)
{
    // --------------------------------------------------------
    // Result init (fail by default)
    // --------------------------------------------------------
    OcrPageResult result;
    result.globalIndex = job.globalIndex;
    result.success = false;
    result.tsvText.clear();
    result.errorMessage.clear();

    auto canceled = [&]() -> bool
    {
        return cancelFlag && cancelFlag->load();
    };

    // --------------------------------------------------------
    // Entry log
    // --------------------------------------------------------
    LogRouter::instance().info(
        QString("[OcrPageWorker] START page=%1 keepInRam=%2 enhancedMat=%3 enhancedPath='%4' dpi=%5 lang='%6'")
            .arg(job.globalIndex)
            .arg(job.keepInRam ? "true" : "false")
            .arg(job.enhancedMat.empty() ? "EMPTY" : "OK")
            .arg(job.enhancedPath)
            .arg(job.ocrDpi)
            .arg(languageString));

    // Early cancel
    if (canceled())
    {
        LogRouter::instance().info(
            QString("[OcrPageWorker] CANCELLED early page=%1").arg(job.globalIndex));
        return result;
    }

    // --------------------------------------------------------
    // Validate language (RUN invariant)
    // --------------------------------------------------------
    const QString languages = languageString.trimmed();
    if (languages.isEmpty())
    {
        LogRouter::instance().error(
            QString("[OcrPageWorker] Page %1: languageString EMPTY (contract violation)")
                .arg(job.globalIndex));

        result.errorMessage = QString("languageString empty for page %1").arg(job.globalIndex);
        return result;
    }

    // =========================================================
    // 1) Acquire input image (CONTRACT-DRIVEN)
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
        const QString path = job.enhancedPath.trimmed();

        if (path.isEmpty())
        {
            LogRouter::instance().error(
                QString("[OcrPageWorker] Page %1: enhancedPath EMPTY (contract violation)")
                    .arg(job.globalIndex));

            result.errorMessage = QString("enhancedPath empty for page %1").arg(job.globalIndex);
            return result;
        }

        if (canceled())
        {
            LogRouter::instance().info(
                QString("[OcrPageWorker] CANCELLED before disk load page=%1")
                    .arg(job.globalIndex));
            return result;
        }

        gray = cv::imread(path.toStdString(), cv::IMREAD_GRAYSCALE);

        LogRouter::instance().info(
            QString("[OcrPageWorker] Page %1: using enhancedPath (DISK) '%2'")
                .arg(job.globalIndex)
                .arg(path));
    }

    // Validate Gray8 image
    if (gray.empty() || gray.type() != CV_8UC1)
    {
        LogRouter::instance().error(
            QString("[OcrPageWorker] Page %1: invalid Gray8 input")
                .arg(job.globalIndex));

        result.errorMessage = QString("Invalid Gray8 input for page %1").arg(job.globalIndex);
        return result;
    }

    if (canceled())
    {
        LogRouter::instance().info(
            QString("[OcrPageWorker] CANCELLED after image acquire page=%1")
                .arg(job.globalIndex));
        return result;
    }

    // =========================================================
    // 2) Read OCR engine parameters (NOT languages)
    // =========================================================
    ConfigManager &cfg = ConfigManager::instance();

    const int oem = cfg.get("ocr.tesseract_oem", 1).toInt();
    const int dpi = job.ocrDpi > 0 ? job.ocrDpi : cfg.get("ocr.dpi_default", 300).toInt();

    // ---------------------------------------------------------
    // Multipass PSM list
    // Reads keys: ocr.psm_1, ocr.psm_2, ...
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

        // Tesseract valid PageSegMode values commonly 0..13
        if (ok && psm >= 0 && psm <= 13)
            psmList << psm;
    }

    if (psmList.isEmpty())
        psmList << 4; // safe default


    const QString tessdataDir =
        OcrLanguageManager::instance().resolvedTessdataDir();

    // =========================================================
    // 3) Multi-pass OCR loop
    // =========================================================
    QList<OcrPassResult> passResults;

    for (int psm : psmList)
    {
        if (canceled())
        {
            LogRouter::instance().info(
                QString("[OcrPageWorker] CANCELLED before pass page=%1 psm=%2")
                    .arg(job.globalIndex)
                    .arg(psm));
            return result;
        }

        OcrPassResult pass;
        pass.config.passName  = QString("psm%1").arg(psm);
        pass.config.languages = languages;
        pass.config.psm       = psm;
        pass.config.oem       = oem;
        pass.config.dpi       = dpi;

        tesseract::TessBaseAPI api;

        // ---------------------------------------------------------
        // Explicit tessdata datapath (parent of "tessdata")
        // ---------------------------------------------------------
        const QString tessdataDir =
            OcrLanguageManager::instance().resolvedTessdataDir();

        const QString datapath = tessdataDir;
        const QByteArray datapathBytes = datapath.toUtf8();
        const QByteArray langBytes     = languages.toUtf8();

        LogRouter::instance().info(
            QString("[OcrPageWorker] Tesseract datapath: %1").arg(datapath));

        for (const QString& code : languages.split('+'))
        {
            const QString p = QDir(tessdataDir).filePath(code + ".traineddata");
            LogRouter::instance().info(
                QString("[OcrPageWorker] traineddata check: %1 exists=%2")
                    .arg(p)
                    .arg(QFile::exists(p) ? "true" : "false"));
        }
        // Init Tesseract
        if (api.Init(datapathBytes.constData(),
                     langBytes.constData(),
                     static_cast<tesseract::OcrEngineMode>(oem)) != 0)
        {
            LogRouter::instance().warning(
                QString("[OcrPageWorker] Page %1: api.Init failed (datapath='%2', lang='%3', oem=%4)")
                    .arg(job.globalIndex)
                    .arg(datapath)
                    .arg(languages)
                    .arg(oem));
            continue;
        }

        // DPI hint (string must remain valid during call)
        const QByteArray dpiBytes = QByteArray::number(dpi);
        api.SetVariable("user_defined_dpi", dpiBytes.constData());

        if (canceled())
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

        if (canceled())
        {
            LogRouter::instance().info(
                QString("[OcrPageWorker] CANCELLED before GetTSVText page=%1")
                    .arg(job.globalIndex));
            return result;
        }

        // Heavy OCR call
        char *raw = api.GetTSVText(0);
        if (!raw)
        {
            LogRouter::instance().warning(
                QString("[OcrPageWorker] Page %1: GetTSVText returned NULL (psm=%2)")
                    .arg(job.globalIndex)
                    .arg(psm));
            continue;
        }

        const QString tsvRaw = QString::fromUtf8(raw);
        delete [] raw;

        pass.tsvText = sanitizeTsvConf(tsvRaw);

        if (canceled())
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
        LogRouter::instance().error(
            QString("[OcrPageWorker] FAIL page=%1: no successful passes").arg(job.globalIndex));

        result.errorMessage = QString("OCR failed for page %1").arg(job.globalIndex);
        return result;
    }

    // =========================================================
    // 4) Select best pass
    // =========================================================
    const OcrPassResult best = selectBestOcrPass(passResults);

    // =========================================================
    // 5) Produce result in RAM
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
