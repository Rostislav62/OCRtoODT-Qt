// ============================================================
//  OCRtoODT — OCR TSV Quality Evaluation
//  File: src/2_ocr/OcrTsvQuality.cpp
//
//  Design notes:
//      • TSV is treated as a hierarchical signal, not text
//      • level 2/3/4/5 correspond to block/paragraph/line/word
//      • conf == -1 rows are structural and ignored for stats
//
//  RAM-first upgrade:
//      • Core logic moved to analyzeTsvQualityFromText()
//      • File-based analyzeTsvQuality() becomes a thin wrapper
// ============================================================

#include "2_ocr/OcrTsvQuality.h"

#include <QFile>
#include <QTextStream>
#include <QStringList>

// ------------------------------------------------------------
// Core: Analyze TSV TEXT in RAM
// ------------------------------------------------------------
OcrTsvQuality analyzeTsvQualityFromText(const QString &tsvText)
{
    OcrTsvQuality q;

    if (tsvText.trimmed().isEmpty())
        return q;

    const QStringList lines =
        tsvText.split('\n', Qt::KeepEmptyParts);

    double confSum = 0.0;
    int    lowConf = 0;
    int    confCount = 0;

    for (const QString &line : lines)
    {
        if (line.isEmpty())
            continue;

        const QStringList cols =
            line.split('\t', Qt::KeepEmptyParts);

        if (cols.size() < 11)
            continue;

        const int level = cols[0].toInt();

        if (level == 2)
            q.blocks++;
        else if (level == 3)
            q.paragraphs++;
        else if (level == 4)
            q.lines++;
        else if (level == 5)
        {
            q.words++;

            // Tesseract conf is in column 10
            // conf can be "-1" (structural marker)
            bool ok = false;
            const double conf = cols[10].toDouble(&ok);

            if (ok && conf >= 0.0)
            {
                confSum += conf;
                confCount++;
                if (conf < 40.0)
                    lowConf++;
            }
        }
    }

    if (confCount > 0)
    {
        q.meanConf = confSum / double(confCount);
        q.lowConfRatio = double(lowConf) / double(confCount);
    }

    // --------------------------------------------------------
    // Structure heuristics (tunable)
    // --------------------------------------------------------
    if (q.words > 150 && q.paragraphs <= 3)
        q.badStructure = true;

    if (q.blocks >= 8 && q.lines <= 40)
        q.badStructure = true;

    if (q.words > 120 && q.lines < 12)
        q.badStructure = true;

    // --------------------------------------------------------
    // Final score (higher is better)
    // --------------------------------------------------------
    q.score =
        (q.words * 0.1)
        + (q.lines * 1.0)
        + (q.paragraphs * 2.0)
        - (q.blocks * 1.0)
        + (q.meanConf * 0.5)
        - (q.lowConfRatio * 50.0);

    if (q.badStructure)
        q.score -= 50.0;

    return q;
}

// ------------------------------------------------------------
// Legacy: Analyze TSV file from disk (wrapper)
// ------------------------------------------------------------
OcrTsvQuality analyzeTsvQuality(const QString &tsvPath)
{
    QFile f(tsvPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return OcrTsvQuality();

    QTextStream in(&f);
    const QString tsvText = in.readAll();
    f.close();

    return analyzeTsvQualityFromText(tsvText);
}
