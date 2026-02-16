// ============================================================
//  OCRtoODT — STEP 3: LineTextBuilder (RAW TSV → LineTable)
//  File: src/3_LineTextBuilder/LineTextBuilder.cpp
// ============================================================

#include "3_LineTextBuilder/LineTextBuilder.h"

#include <QStringList>
#include <QtGlobal>
#include <algorithm>

namespace Tsv {

// ------------------------------------------------------------
// Utility: median
// ------------------------------------------------------------
int LineTextBuilder::medianInt(QVector<int> v)
{
    if (v.isEmpty())
        return 0;

    std::sort(v.begin(), v.end());
    const int mid = v.size() / 2;
    if (v.size() % 2 == 1)
        return v[mid];

    // even
    return (v[mid - 1] + v[mid]) / 2;
}

// ------------------------------------------------------------
// TSV line parser (12 columns expected)
// level page block par line word left top width height conf text
// ------------------------------------------------------------
bool LineTextBuilder::parseTsvLine(const QString &line, TsvRow *out)
{
    if (!out)
        return false;

    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty())
        return false;

    const QStringList cols = trimmed.split('\t');
    if (cols.size() < 11)
        return false;

    // Header line usually begins with "level"
    if (cols[0].toLower() == "level")
        return false;

    bool ok = false;

    auto toInt = [&](int idx, int def = -1) -> int {
        if (idx < 0 || idx >= cols.size())
            return def;
        int v = cols[idx].toInt(&ok);
        return ok ? v : def;
    };

    auto toDouble = [&](int idx, double def = -1.0) -> double {
        if (idx < 0 || idx >= cols.size())
            return def;
        double v = cols[idx].toDouble(&ok);
        return ok ? v : def;
    };

    out->level  = toInt(0);
    out->page   = toInt(1);
    out->block  = toInt(2);
    out->par    = toInt(3);
    out->line   = toInt(4);
    out->word   = toInt(5);

    out->left   = toInt(6, 0);
    out->top    = toInt(7, 0);
    out->width  = toInt(8, 0);
    out->height = toInt(9, 0);

    out->conf   = toDouble(10, -1.0);

    // text (may contain tabs only if TSV is malformed; we assume standard TSV)
    if (cols.size() >= 12)
        out->text = cols[11];
    else
        out->text.clear();

    return true;
}

// ------------------------------------------------------------
// Word join with minimal punctuation heuristic
// ------------------------------------------------------------
QString LineTextBuilder::joinWordWithSpacing(const QString &current,
                                             const QString &nextWord)
{
    if (current.isEmpty())
        return nextWord;

    if (nextWord.isEmpty())
        return current;

    // If nextWord begins with punctuation, avoid leading space.
    const QChar c = nextWord[0];
    const QString punct = ".,;:!?)]}%»…";
    if (punct.contains(c))
        return current + nextWord;

    // If current ends with opening punctuation, avoid space after it.
    const QChar last = current[current.size() - 1];
    const QString openPunct = "([{\"«";
    if (openPunct.contains(last))
        return current + nextWord;

    return current + " " + nextWord;
}

// ------------------------------------------------------------
// Main builder
// ------------------------------------------------------------
LineTable* LineTextBuilder::build(const Core::VirtualPage &vp,
                                  const QString          &tsvText)
{
    // Caller owns LineTable (stored inside vp.lineTable)
    LineTable *table = new LineTable();

    if (tsvText.trimmed().isEmpty())
        return table;

    // --------------------------------------------------------
    // First pass: collect line (level 4) rows in TSV order
    // and accumulate words (level 5) → text, avgConf, wordCount.
    //
    // TSV rules:
    //  - use levels 2/3/4 for structure markers
    //  - ignore level 1 and level 5 as "line source"
    //  - BUT level 5 is used for text + metrics accumulation
    // --------------------------------------------------------

    struct Acc
    {
        QString text;
        double  confSum = 0.0;
        int     words   = 0;
    };

    // Key = block|par|line
    auto makeKey = [](int b, int p, int l) -> qint64 {
        // pack into 3x 21-bit (safe for typical TSV ranges)
        return (qint64(b) << 42) | (qint64(p) << 21) | qint64(l);
    };

    QHash<qint64, Acc> accByLine;

    // We keep the level4 order list for later gap-based empty lines synthesis
    struct LineStub
    {
        int   block = -1;
        int   par   = -1;
        int   line  = -1;
        QRect bbox;
    };
    QVector<LineStub> stubs;

    const QStringList lines = tsvText.split('\n');

    for (const QString &raw : lines)
    {
        TsvRow r;
        if (!parseTsvLine(raw, &r))
            continue;

        // ignore level 1 and other irrelevant levels
        if (r.level == 4)
        {
            LineStub s;
            s.block = r.block;
            s.par   = r.par;
            s.line  = r.line;
            s.bbox  = QRect(r.left, r.top, r.width, r.height);

            stubs.push_back(s);
        }
        else if (r.level == 5)
        {
            // word row: attach to its parent line by (block,par,line)
            const qint64 key = makeKey(r.block, r.par, r.line);

            Acc &a = accByLine[key];

            const QString w = r.text;
            if (!w.isEmpty())
                a.text = joinWordWithSpacing(a.text, w);

            // conf can be -1 on some structural rows, but level 5 usually has real conf
            if (r.conf >= 0.0)
                a.confSum += r.conf;

            // Count only real (non-empty) words
            if (!w.trimmed().isEmpty())
                a.words += 1;
        }
        else
        {
            // level 2/3 are structural markers, not directly needed here,
            // but we keep them implicitly via block/par/line ids.
            continue;
        }
    }

    // --------------------------------------------------------
    // Build initial LineRow list (without empty line synthesis)
    // --------------------------------------------------------
    QVector<int> heights;
    heights.reserve(stubs.size());

    for (const LineStub &s : stubs)
        heights.push_back(qMax(0, s.bbox.height()));

    const int medianH = qMax(1, medianInt(heights));

    // Gap threshold heuristic:
    // - If the vertical gap exceeds ~1.2 * median line height,
    //   insert ONE empty line.
    const int gapThreshold = qMax(6, int(double(medianH) * 1.20));

    int order = 0;

    for (int i = 0; i < stubs.size(); ++i)
    {
        const LineStub &cur = stubs[i];

        // ---- synthesize empty line by bbox vertical gap ----
        if (i > 0)
        {
            const LineStub &prev = stubs[i - 1];

            const int prevBottom = prev.bbox.y() + prev.bbox.height();
            const int gap = cur.bbox.y() - prevBottom;

            if (gap > gapThreshold)
            {
                LineRow empty;
                empty.pageIndex = vp.globalIndex;
                empty.lineOrder = order++;
                empty.text      = "";
                empty.bbox      = QRect(); // no geometry for synthetic empty line
                empty.blockNum  = -1;
                empty.parNum    = -1;
                empty.lineNum   = -1;
                empty.avgConf   = 0.0;
                empty.wordCount = 0;

                table->rows.push_back(empty);
            }
        }

        // ---- real line row ----
        LineRow row;
        row.pageIndex = vp.globalIndex;
        row.lineOrder = order++;
        row.bbox      = cur.bbox;
        row.blockNum  = cur.block;
        row.parNum    = cur.par;
        row.lineNum   = cur.line;

        const qint64 key = makeKey(cur.block, cur.par, cur.line);
        const Acc a = accByLine.value(key);

        row.text      = a.text;
        row.wordCount = a.words;

        if (a.words > 0)
            row.avgConf = a.confSum / double(a.words);
        else
            row.avgConf = 0.0;

        table->rows.push_back(row);
    }

    return table;
}

} // namespace Tsv
