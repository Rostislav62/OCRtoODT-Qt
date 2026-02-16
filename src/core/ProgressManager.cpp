// ============================================================
//  OCRtoODT — Progress Manager
//  File: core/progressmanager.cpp
// ============================================================

#include "core/ProgressManager.h"

namespace Core {

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
ProgressManager::ProgressManager(QObject *parent)
    : QObject(parent)
{
}

// ------------------------------------------------------------
// startPipeline
// ------------------------------------------------------------
void ProgressManager::startPipeline(int totalStages, int totalUnitsHint)
{
    Q_UNUSED(totalUnitsHint);

    m_totalStages       = qMax(0, totalStages);
    m_stageCount        = m_totalStages;
    m_currentStageIndex = -1;
    m_totalUnitsInStage = 0;
    m_unitsDoneInStage  = 0;
    //m_globalMaximum     = m_totalStages * 100;
    m_globalMaximum     = 100;
    m_currentStageName.clear();

    emit progressChanged(0, 100, QString());

    emitProgress(QStringLiteral("Pipeline started"));

    m_pipelineTimer.start();

}

// ------------------------------------------------------------
// startStage
// ------------------------------------------------------------
void ProgressManager::startStage(const QString &stageName,
                                 int stageIndex,
                                 int stageCount,
                                 int totalUnits)
{
    m_stageTimer.start();

    m_currentStageName  = stageName;
    m_currentStageIndex = qBound(0, stageIndex, qMax(0, stageCount - 1));
    m_stageCount        = qMax(1, stageCount);
    m_totalUnitsInStage = qMax(1, totalUnits);
    m_unitsDoneInStage  = 0;

    emit stageChanged(m_currentStageName,
                      m_currentStageIndex,
                      m_stageCount);

    emitProgress(QStringLiteral("%1 (0/%2)")
                     .arg(m_currentStageName)
                     .arg(m_totalUnitsInStage));
}

// ------------------------------------------------------------
// advance
// ------------------------------------------------------------
void ProgressManager::advance(int units)
{
    if (m_totalStages <= 0)
        return;

    m_unitsDoneInStage += units;
    if (m_unitsDoneInStage > m_totalUnitsInStage)
        m_unitsDoneInStage = m_totalUnitsInStage;

    emitProgress();
}

// ------------------------------------------------------------
// finishStage
// ------------------------------------------------------------
void ProgressManager::finishStage()
{
    if (m_totalStages <= 0)
        return;

    m_unitsDoneInStage = m_totalUnitsInStage;
    emitProgress(QStringLiteral("%1 — done").arg(m_currentStageName));
}

// ------------------------------------------------------------
// finishPipeline
// ------------------------------------------------------------
void ProgressManager::finishPipeline(bool ok,
                                     const QString &message)
{
    if (m_totalStages > 0)
    {
        m_currentStageIndex = m_totalStages - 1;
        m_unitsDoneInStage  = m_totalUnitsInStage;
    }

    QString finalText;

    if (!message.isEmpty())
    {
        finalText = message;
    }
    else
    {
        finalText = ok ? tr("Completed")
                       : tr("Cancelled");
    }

    if (ok)
    {
        // Only show execution time for successful completion
        qint64 ms = m_stageTimer.elapsed();
        finalText += QString(" (%1 ms)").arg(ms);
    }

    emitProgress(finalText);

    emit pipelineFinished(ok, finalText);
}


// ------------------------------------------------------------
// emitProgress
// ------------------------------------------------------------
void ProgressManager::emitProgress(const QString &extraText)
{
    // --------------------------------------------------------
    // Если пайплайн не инициализирован
    // --------------------------------------------------------
    if (m_totalStages <= 0)
    {
        emit progressChanged(0, 100,
                             extraText.isEmpty()
                                 ? QStringLiteral("Idle")
                                 : extraText);
        return;
    }

    // --------------------------------------------------------
    // Защита от выхода индекса стадии за границы
    // --------------------------------------------------------
    int stageIndex = m_currentStageIndex;
    if (stageIndex < 0)
        stageIndex = 0;
    if (stageIndex >= m_totalStages)
        stageIndex = m_totalStages - 1;

    // --------------------------------------------------------
    // Процент выполнения внутри текущей стадии (0–100)
    // --------------------------------------------------------
    int percentInStage = 0;
    if (m_totalUnitsInStage > 0)
    {
        percentInStage =
            (m_unitsDoneInStage * 100) / m_totalUnitsInStage;
    }

    // --------------------------------------------------------
    // Каждая стадия занимает равную часть от 100%
    // --------------------------------------------------------
    const int stageWeight = 100 / m_totalStages;

    // Базовое смещение стадии
    int value = stageIndex * stageWeight;

    // Добавляем прогресс внутри стадии
    value += (percentInStage * stageWeight) / 100;

    // --------------------------------------------------------
    // Текст для UI
    // --------------------------------------------------------
    QString text;

    if (!extraText.isEmpty())
    {
        text = extraText;
    }
    else
    {
        text = QStringLiteral("%1 (%2/%3)")
        .arg(m_currentStageName)
            .arg(m_unitsDoneInStage)
            .arg(m_totalUnitsInStage);

        // --------------------------------------------------------
        // ETA calculation (only when some progress exists)
        // --------------------------------------------------------
        if (m_unitsDoneInStage > 0 && m_totalUnitsInStage > 0)
        {
            qint64 elapsedMs = m_pipelineTimer.elapsed();

            // average time per unit
            double timePerUnit =
                double(elapsedMs) / m_unitsDoneInStage;

            int remainingUnits =
                m_totalUnitsInStage - m_unitsDoneInStage;

            qint64 etaMs =
                qint64(timePerUnit * remainingUnits);

            text += QStringLiteral(" | ETA %1 s")
                        .arg(etaMs / 1000);
        }
    }


    emit progressChanged(value, 100, text);
}

void ProgressManager::reset()
{
    m_totalStages        = 0;
    m_currentStageIndex  = -1;
    m_totalUnitsInStage  = 0;
    m_unitsDoneInStage   = 0;
    m_currentStageName.clear();

    emit progressChanged(0, 100, tr("Ready"));
}



} // namespace Core
