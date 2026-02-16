// ============================================================
//  OCRtoODT — Preprocess Pipeline (Policy-Driven, Parallel Engine)
//  File: src/1_preprocess/Preprocess_Pipeline.h
//
//  Responsibility:
//      Coordinate full preprocessing of all input pages
//      using a parallel, per-page pipeline:
//
//          For each VirtualPage (in parallel):
//              EnhanceProcessor::processSingle()
//
//  Features:
//      • QtConcurrent::mapped for automatic multithreading
//      • Respects general.parallel_enabled / general.num_processes
//      • Preserves page order by globalIndex
//      • Emits progress events (optional use by UI)
//      • Returns a vector<PageJob>
//
//  RAM/Disk policy (FINAL):
//      • All policy decisions are based ONLY on:
//            - general.mode
//            - general.debug_mode
//
//      • EnhanceProcessor NEVER writes to disk
//      • PreprocessPipeline is the ONLY place where:
//            - disk output is performed
//            - keepInRam / savedToDisk are decided
//
// ============================================================

#ifndef PREPROCESS_PIPELINE_H
#define PREPROCESS_PIPELINE_H

#include <QObject>
#include <QVector>

#include "core/VirtualPage.h"
#include "1_preprocess/PageJob.h"
#include "1_preprocess/EnhanceProcessor.h"

namespace Ocr {
namespace Preprocess {

class PreprocessPipeline : public QObject
{
    Q_OBJECT

public:
    explicit PreprocessPipeline(QObject *parent = nullptr);

    // ------------------------------------------------------------
    //  Main API — process all pages (parallel)
    // ------------------------------------------------------------
    QVector<PageJob> run(const QVector<Core::VirtualPage> &pages);

signals:
    void progressChanged(int value);
    void message(QString msg);

private:
    EnhanceProcessor m_processor;
    int m_threadCount = 1;

    void configureThreadPool();
};

} // namespace Preprocess
} // namespace Ocr

#endif // PREPROCESS_PIPELINE_H
