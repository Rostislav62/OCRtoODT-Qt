// ============================================================
//  OCRtoODT — Progress Manager
//  File: core/progressmanager.h
//
//  Responsibility:
//      Lightweight helper for reporting progress of multi-stage
//      operations (like OCR pipeline) to the GUI.
//
//      Features:
//          - tracks pipeline composed of N stages
//          - each stage has a name and "units of work" (pages)
//          - computes global progress [0 .. max] where each
//            stage has equal weight
//          - emits convenient Qt signals for UI:
//
//              progressChanged(value, maximum, text)
//              stageChanged(stageName, stageIndex, stageCount)
//              pipelineFinished(ok, message)
//
//      NOTE:
//          This class has no knowledge about OCR itself. Any
//          long-running process may use it.
// ============================================================

#ifndef CORE_PROGRESSMANAGER_H
#define CORE_PROGRESSMANAGER_H

#include <QObject>
#include <QString>
#include <QElapsedTimer>



namespace Core {

// ============================================================
//  ProgressManager
// ============================================================
class ProgressManager : public QObject
{
    Q_OBJECT

public:
    explicit ProgressManager(QObject *parent = nullptr);

    // --------------------------------------------------------
    // Initialize pipeline-level tracking.
    //
    //  totalStages     — number of stages (Normalize, Enhance, OCR...)
    //  totalUnitsHint  — optional hint for UI; not used in math,
    //                    but can be shown in text.
    // --------------------------------------------------------
    void startPipeline(int totalStages, int totalUnitsHint = 0);

    // --------------------------------------------------------
    // Start specific stage.
    //
    //  stageName   — human readable name ("Step 1.1 — Normalize")
    //  stageIndex  — 0-based index
    //  stageCount  — total stages (for information)
    //  totalUnits  — "units of work" in this stage (pages, jobs)
    // --------------------------------------------------------
    void startStage(const QString &stageName,
                    int stageIndex,
                    int stageCount,
                    int totalUnits);

    // Increase progress in current stage by given units (default 1).
    void advance(int units = 1);

    // Finish current stage (sets progress to end of stage).
    void finishStage();

    // Finish pipeline (global).
    void finishPipeline(bool ok, const QString &message);

    void reset();


signals:
    // Global numerical progress for GUI (QProgressBar etc.)
    void progressChanged(int value, int maximum, const QString &text);

    // Informational: current stage changed.
    void stageChanged(const QString &stageName,
                      int stageIndex,
                      int stageCount);

    // Pipeline finished (ok / error).
    void pipelineFinished(bool ok, const QString &message);

private:
    int     m_totalStages         = 0;
    int     m_currentStageIndex   = -1;
    int     m_stageCount          = 0;
    int     m_totalUnitsInStage   = 0;
    int     m_unitsDoneInStage    = 0;
    int     m_globalMaximum       = 0;   // totalStages * 100
    QString m_currentStageName;

    QElapsedTimer m_stageTimer;
    QElapsedTimer m_pipelineTimer;

    void emitProgress(const QString &extraText = QString());
};

} // namespace Core

#endif // CORE_PROGRESSMANAGER_H
