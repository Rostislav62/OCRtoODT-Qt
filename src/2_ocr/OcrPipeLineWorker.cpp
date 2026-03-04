// ============================================================
//  OCRtoODT — OCR Pipeline Worker
//  File: src/2_ocr/OcrPipeLineWorker.cpp
//
//  Responsibility:
//      STEP 2 — OCR execution worker (RAM-first).
//
//  Architecture:
//      • Receives normalized PageJob list from Controller
//      • Executes OCR per page in parallel (QtConcurrent)
//      • Collects results in deterministic globalIndex order
//      • NEVER reads ConfigManager for languages
//      • Language string is RUN invariant (injected by Controller)
//
//  Guarantees:
//      • Stable mapping by globalIndex
//      • RAM is source of truth
//      • Disk output ONLY inside OcrPageWorker if required
//      • Safe cooperative cancellation
//      • Deterministic finish semantics
//
// ============================================================

#include "2_ocr/OcrPipeLineWorker.h"

#include <QtConcurrent>
#include <QFutureWatcher>
#include <QThread>

#include "core/LogRouter.h"
#include "2_ocr/OcrPageWorker.h"

using namespace Ocr;

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
OcrPipelineWorker::OcrPipelineWorker(QObject *parent)
    : QObject(parent)
{
}

// ------------------------------------------------------------
// Start OCR pipeline (STEP 2)
// ------------------------------------------------------------
void OcrPipelineWorker::start(
    const QVector<Ocr::Preprocess::PageJob> &jobs,
    const QString& mode,
    bool debug,
    const QString& languageString,
    const std::atomic_bool *cancelFlag)
{
    LogRouter::instance().info(
        QString("[STATE] run=%1 WORKER event=START pages=%2")
            .arg(m_runId)
            .arg(jobs.size()));

    // --------------------------------------------------------
    // Store RUN invariants
    // --------------------------------------------------------
    m_jobs           = jobs;
    m_mode           = mode;
    m_debugMode      = debug;
    m_languageString = languageString;   // CRITICAL: resolved by Controller
    m_cancelFlag     = cancelFlag;

    // --------------------------------------------------------
    // Defensive: prevent overlapping futures
    // --------------------------------------------------------
    if (m_future.isRunning())
    {
        LogRouter::instance().warning(
            "[OcrPipelineWorker] start() called while previous future is running. Cancelling previous run...");

        m_future.cancel();
        m_future.waitForFinished();
    }

    const int total = m_jobs.size();

    if (total == 0)
    {
        emit ocrMessage("No pages for OCR.");

        LogRouter::instance().info(
            QString("[STATE] run=%1 WORKER event=EMIT_FINISHED")
                .arg(m_runId));

        emit ocrFinished();
        emit ocrCompleted({});
        return;
    }

    LogRouter::instance().info(
        QString("[OcrPipelineWorker] OCR started. Pages=%1 Mode=%2 Debug=%3 Lang=%4")
            .arg(total)
            .arg(m_mode)
            .arg(m_debugMode ? "true" : "false")
            .arg(m_languageString));

    // =========================================================
    // STEP 2A — normalize jobs by globalIndex (CRITICAL)
    //
    // Guarantee deterministic index mapping even if input order
    // was altered upstream.
    // =========================================================
    QVector<Ocr::Preprocess::PageJob> jobsByIndex;
    jobsByIndex.resize(total);

    for (const auto &job : m_jobs)
    {
        const int gi = job.globalIndex;

        if (gi < 0 || gi >= total)
        {
            LogRouter::instance().warning(
                QString("[OcrPipelineWorker] Invalid globalIndex=%1")
                    .arg(gi));
            continue;
        }

        jobsByIndex[gi] = job;
    }

    // =========================================================
    // STEP 2B — Parallel OCR execution
    //
    // IMPORTANT:
    //   • Worker NEVER resolves languages itself.
    //   • PageWorker receives languageString directly.
    // =========================================================
    auto lambdaOcr =
        [this](const Ocr::Preprocess::PageJob &job) -> OcrPageResult
    {
        // ----------------------------------------------------
        // Fast exit if cancelled before processing this page
        // ----------------------------------------------------
        if (m_cancelFlag && m_cancelFlag->load())
        {
            OcrPageResult r;
            r.globalIndex = job.globalIndex;
            r.success = false;
            r.tsvText.clear();
            return r;
        }

        return OcrPageWorker::run(
            job,
            m_languageString,
            m_cancelFlag);
    };

    m_future = QtConcurrent::mapped(jobsByIndex, lambdaOcr);

    QFutureWatcher<OcrPageResult> *watcher =
        new QFutureWatcher<OcrPageResult>(this);

    // --------------------------------------------------------
    // Progress reporting
    // --------------------------------------------------------
    connect(watcher,
            &QFutureWatcher<OcrPageResult>::progressValueChanged,
            this,
            [this, total](int value)
            {
                emit ocrProgress(value, total);
            });

    // --------------------------------------------------------
    // Finish handler
    //
    // CRITICAL:
    //   After cancel(), future may produce fewer results
    //   than total pages.
    // =========================================================
    connect(watcher,
            &QFutureWatcher<OcrPageResult>::finished,
            this,
            [this, watcher, jobsByIndex, total]()
            {
                const QFuture<OcrPageResult> future = watcher->future();

                QVector<Core::VirtualPage> pages;
                pages.resize(total);

                // ------------------------------------------------
                // Initialize all pages as failed by default
                // ------------------------------------------------
                for (int gi = 0; gi < total; ++gi)
                {
                    Core::VirtualPage vp = jobsByIndex[gi].vp;
                    vp.ocrSuccess = false;
                    vp.ocrTsvText.clear();
                    pages[gi] = vp;
                }

                int okCount   = 0;
                int failCount = 0;

                const QList<OcrPageResult> results = future.results();

                LogRouter::instance().info(
                    QString("[OcrPipelineWorker] finished: canceled=%1 produced=%2 total=%3")
                        .arg(future.isCanceled() ? "true" : "false")
                        .arg(results.size())
                        .arg(total));

                // ------------------------------------------------
                // Merge produced results
                // ------------------------------------------------
                for (const OcrPageResult &r : results)
                {
                    const int gi = r.globalIndex;

                    if (gi < 0 || gi >= total)
                        continue;

                    Core::VirtualPage vp = pages[gi];
                    vp.ocrSuccess = r.success;

                    if (r.success)
                    {
                        ++okCount;
                        vp.ocrTsvText = r.tsvText;
                    }
                    else
                    {
                        ++failCount;
                    }

                    pages[gi] = vp;
                }

                // ------------------------------------------------
                // CANCELED path
                //
                // We DO NOT emit ocrCompleted() here.
                // Controller will handle idle transition.
                // ------------------------------------------------
                if ((m_cancelFlag && m_cancelFlag->load()) ||
                    future.isCanceled())
                {
                    LogRouter::instance().warning(
                        QString("[OcrPipelineWorker] OCR finished in CANCELED state. produced=%1 total=%2 ok=%3 fail=%4")
                            .arg(results.size())
                            .arg(total)
                            .arg(okCount)
                            .arg(failCount));

                    emit ocrFinished();
                    watcher->deleteLater();
                    return;
                }

                // ------------------------------------------------
                // NORMAL completion path
                // ------------------------------------------------
                emit ocrFinished();
                emit ocrCompleted(pages);

                watcher->deleteLater();
            });

    watcher->setFuture(m_future);
}

// ------------------------------------------------------------
// Cancel (cooperative)
// ------------------------------------------------------------
void OcrPipelineWorker::cancel()
{
    LogRouter::instance().warning(
        "[OcrPipelineWorker] cancel() requested");

    if (m_future.isRunning())
        m_future.cancel();

    emit ocrMessage(tr("Cancellation requested..."));
}

// ------------------------------------------------------------
// Safe shutdown wait
//
// Guarantees:
//   • No QtConcurrent task survives object destruction
// ------------------------------------------------------------
void OcrPipelineWorker::waitForFinished()
{
    if (m_future.isRunning())
    {
        LogRouter::instance().info(
            "[OcrPipelineWorker] waitForFinished(): waiting for QtConcurrent future...");

        m_future.waitForFinished();

        LogRouter::instance().info(
            "[OcrPipelineWorker] waitForFinished(): future finished.");
    }
}
