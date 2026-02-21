// ============================================================
//  OCRtoODT — OCR Pipeline Worker
//  File: src/2_ocr/OcrPipeLineWorker.cpp
//
//  Responsibility:
//      STEP 2 — OCR execution worker (RAM-first).
//
//      Guarantees:
//          • Stable mapping by globalIndex
//          • RAM is source of truth
//          • Disk output ONLY in debug / disk_only
//
// ============================================================

#include "2_ocr/OcrPipeLineWorker.h"

#include <QtConcurrent>
#include <QFutureWatcher>
#include <QDir>
#include <QFile>
#include <QTextStream>

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
void OcrPipelineWorker::start(const QVector<Ocr::Preprocess::PageJob> &jobs,
                              const QString& mode,
                              bool debug,
                              const std::atomic_bool *cancelFlag)
{
    m_jobs      = jobs;
    m_mode      = mode;
    m_debugMode = debug;

    m_cancelFlag = cancelFlag;

    // --------------------------------------------------------
    // Defensive: never overlap futures.
    // If a previous run is still finishing, request cancel and wait.
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
        emit ocrFinished();
        emit ocrCompleted({});
        return;
    }

    LogRouter::instance().info(
        QString("[OcrPipelineWorker] OCR started. Pages=%1 Mode=%2 Debug=%3")
            .arg(total)
            .arg(m_mode)
            .arg(m_debugMode));

    // =========================================================
    // STEP 2A — normalize jobs by globalIndex (CRITICAL)
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
    // STEP 2B — OCR per page (PARALLEL)
    // =========================================================
    auto lambdaOcr =
        [this](const Ocr::Preprocess::PageJob &job) -> OcrPageResult
    {
        // Fast exit if cancelled before starting this job
        if (m_cancelFlag && m_cancelFlag->load())
        {
            OcrPageResult r;
            r.globalIndex = job.globalIndex;
            r.success = false;
            r.tsvText.clear();
            return r;
        }

        // Cooperative cancel inside page worker too
        return OcrPageWorker::run(job, m_cancelFlag);
    };



    m_future = QtConcurrent::mapped(jobsByIndex, lambdaOcr);


    QFutureWatcher<OcrPageResult> *watcher =
        new QFutureWatcher<OcrPageResult>(this);

    connect(watcher,
            &QFutureWatcher<OcrPageResult>::progressValueChanged,
            this,
            [this, total](int value)
            {
                emit ocrProgress(value, total);
            });

    // File: src/2_ocr/OcrPipeLineWorker.cpp
    connect(watcher,
            &QFutureWatcher<OcrPageResult>::finished,
            this,
            [this, watcher, jobsByIndex, total]()
            {
                const QFuture<OcrPageResult> future = watcher->future();

                // --------------------------------------------------------
                // Build default page array first (all failed by default).
                // We MUST NOT assume future has 'total' results after cancel().
                // --------------------------------------------------------
                QVector<Core::VirtualPage> pages;
                pages.resize(total);

                for (int gi = 0; gi < total; ++gi)
                {
                    Core::VirtualPage vp = jobsByIndex[gi].vp;
                    vp.ocrSuccess = false;
                    vp.ocrTsvText.clear();
                    pages[gi] = vp;
                }

                int okCount   = 0;
                int failCount = 0;

                // --------------------------------------------------------
                // Collect only produced results.
                // NOTE: after cancel(), resultCount() may be < total.
                // --------------------------------------------------------
                const QList<OcrPageResult> results = future.results();

                LogRouter::instance().info(
                    QString("[OcrPipelineWorker] finished: canceled=%1 produced=%2 total=%3")
                        .arg(future.isCanceled() ? "true" : "false")
                        .arg(results.size())
                        .arg(total));


                for (const OcrPageResult &r : results)
                {
                    const int gi = r.globalIndex;

                    if (gi < 0 || gi >= total)
                        continue;

                    Core::VirtualPage vp = pages[gi]; // already has base VP
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

                // --------------------------------------------------------
                // If cancellation happened — do NOT treat as success.
                // We only notify finish, but do NOT emit completed result.
                // --------------------------------------------------------
                if ((m_cancelFlag && m_cancelFlag->load()) || future.isCanceled())
                {
                    LogRouter::instance().warning(
                        QString("[OcrPipelineWorker] OCR finished in CANCELED state. produced=%1 total=%2 ok=%3 fail=%4")
                            .arg(results.size())
                            .arg(total)
                            .arg(okCount)
                            .arg(failCount));

                    emit ocrFinished();        // stage finished
                    watcher->deleteLater();
                    return;                    // 🚨 EXIT — DO NOT emit ocrCompleted
                }

                // --------------------------------------------------------
                // Normal completion path
                // --------------------------------------------------------
                emit ocrFinished();
                emit ocrCompleted(pages);

                watcher->deleteLater();

            });


    watcher->setFuture(m_future);
}

void OcrPipelineWorker::cancel()
{
    // --------------------------------------------------------
    // Cooperative cancel request.
    // NOTE:
    //  - OcrPipelineController already invokes this slot via QueuedConnection,
    //    so this code normally executes in the worker thread.
    //  - We do NOT block/wait here. We only request cancellation.
    // --------------------------------------------------------
    LogRouter::instance().warning("[OcrPipelineWorker] cancel() requested");


    // NOTE:
    // Cancel token is owned by Controller. We only cancel the future here.
    // QtConcurrent cancellation is cooperative (not immediate).
    if (m_future.isRunning())
        m_future.cancel();

    emit ocrMessage(tr("Cancellation requested..."));
}

void OcrPipelineWorker::waitForFinished()
{
    // --------------------------------------------------------
    // STEP 4.2 — Safe shutdown wait.
    // QtConcurrent cancellation is cooperative; this call ensures
    // all tasks have completed before object destruction.
    // --------------------------------------------------------
    if (m_future.isRunning())
    {
        LogRouter::instance().info(
            "[OcrPipelineWorker] waitForFinished(): waiting for QtConcurrent future...");

        m_future.waitForFinished();

        LogRouter::instance().info(
            "[OcrPipelineWorker] waitForFinished(): future finished.");
    }
}
