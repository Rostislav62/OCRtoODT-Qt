// ============================================================
//  OCRtoODT — STEP 3: LineTableSerializer
//  File: src/3_LineTextBuilder/LineTableSerializer.cpp
// ============================================================

#include "3_LineTextBuilder/LineTableSerializer.h"

#include <QFile>
#include <QTextStream>
#include <QDir>

namespace Tsv {

// ------------------------------------------------------------
// Save LineTable → internal TSV
// ------------------------------------------------------------
bool LineTableSerializer::saveToTsv(const LineTable &table,
                                    const QString  &filePath,
                                    QString        *error)
{
    QDir().mkpath(QFileInfo(filePath).absolutePath());

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        if (error) *error = "Cannot open file for writing: " + filePath;
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Header (for humans)
    out << "pageIndex\tlineOrder\tblockNum\tparNum\tlineNum\t"
           "left\ttop\tright\tbottom\tavgConf\twordCount\ttext\n";

    for (const LineRow &r : table.rows)
    {
        out << r.pageIndex << '\t'
            << r.lineOrder << '\t'
            << r.blockNum << '\t'
            << r.parNum << '\t'
            << r.lineNum << '\t'
            << r.bbox.left() << '\t'
            << r.bbox.top() << '\t'
            << r.bbox.right() << '\t'
            << r.bbox.bottom() << '\t'
            << r.avgConf << '\t'
            << r.wordCount << '\t'
            << QString(r.text).replace('\n', ' ')
            << '\n';
    }

    return true;
}

// ------------------------------------------------------------
// Load LineTable ← internal TSV
// ------------------------------------------------------------
LineTable* LineTableSerializer::loadFromTsv(const QString &filePath,
                                            QString       *error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (error) *error = "Cannot open file for reading: " + filePath;
        return nullptr;
    }

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);

    LineTable *table = new LineTable();

    bool isHeader = true;
    while (!in.atEnd())
    {
        const QString line = in.readLine();
        if (line.trimmed().isEmpty())
            continue;

        if (isHeader)
        {
            isHeader = false;
            continue;
        }

        const QStringList c = line.split('\t');
        if (c.size() < 12)
            continue;

        LineRow r;
        r.pageIndex  = c[0].toInt();
        r.lineOrder  = c[1].toInt();
        r.blockNum   = c[2].toInt();
        r.parNum     = c[3].toInt();
        r.lineNum    = c[4].toInt();

        const int left   = c[5].toInt();
        const int top    = c[6].toInt();
        const int right  = c[7].toInt();
        const int bottom = c[8].toInt();
        r.bbox = QRect(QPoint(left, top),
                       QPoint(right, bottom));

        r.avgConf   = c[9].toDouble();
        r.wordCount = c[10].toInt();
        r.text      = c[11];

        table->rows.push_back(r);
    }

    return table;
}

} // namespace Tsv
