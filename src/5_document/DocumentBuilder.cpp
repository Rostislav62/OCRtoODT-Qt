// ============================================================
//  OCRtoODT — STEP 5.2: DocumentBuilder (LineTable → DocumentModel)
//  File: src/5_document/DocumentBuilder.cpp
// ============================================================

#include "5_document/DocumentBuilder.h"

#include <algorithm>

#include "3_LineTextBuilder/LineTable.h"
#include "core/LogRouter.h"

namespace Step5 {

DocumentModel DocumentBuilder::build(const QVector<Core::VirtualPage> &pages,
                                     const DocumentBuildOptions       &opt)
{
    DocumentModel doc;
    doc.options = opt;

    const QVector<Core::VirtualPage> ordered = sortedByGlobalIndex(pages);

    int pagesUsed = 0;

    for (int i = 0; i < ordered.size(); ++i)
    {
        const Core::VirtualPage &vp = ordered[i];

        if (!vp.lineTable)
        {
            LogRouter::instance().warning(
                QString("[STEP 5.2] Skip page=%1 (lineTable=null)")
                    .arg(vp.globalIndex));
            continue;
        }

        // ------------------------------------------------------------
        // IMPORTANT:
        // Page breaks are no longer stored as DocumentBlock entries.
        // Exporters derive page breaks by comparing pageIndex transitions.
        // ------------------------------------------------------------
        appendPageAsBlocks(vp, opt, &doc, i);

        ++pagesUsed;
    }

    LogRouter::instance().info(
        QString("[STEP 5.2] Document built: pagesUsed=%1 blocks=%2 policy=%3")
            .arg(pagesUsed)
            .arg(doc.blocks.size())
            .arg(static_cast<int>(opt.paragraphPolicy)));

    return doc;
}

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------

QVector<Core::VirtualPage> DocumentBuilder::sortedByGlobalIndex(
    const QVector<Core::VirtualPage> &pages)
{
    QVector<Core::VirtualPage> out = pages;

    std::sort(out.begin(), out.end(),
              [](const Core::VirtualPage &a, const Core::VirtualPage &b)
              {
                  return a.globalIndex < b.globalIndex;
              });

    return out;
}

void DocumentBuilder::appendPageAsBlocks(const Core::VirtualPage    &vp,
                                         const DocumentBuildOptions &opt,
                                         DocumentModel              *out,
                                         int                         pageIndex)
{
    if (!out)
        return;

    switch (opt.paragraphPolicy)
    {
    case ParagraphPolicy::MVP_LinePerParagraph:
        appendPage_MvpLinePerParagraph(vp, opt, out, pageIndex);
        break;

    case ParagraphPolicy::FromStep3Markers:
    default:
        appendPage_FromStep3Markers(vp, opt, out, pageIndex);
        break;
    }
}

void DocumentBuilder::appendPage_MvpLinePerParagraph(
    const Core::VirtualPage    &vp,
    const DocumentBuildOptions &opt,
    DocumentModel              *out,
    int                         pageIndex)
{
    const Tsv::LineTable *t = vp.lineTable;
    if (!t)
        return;

    int emptyRun = 0;

    for (const auto &row : t->rows)
    {
        if (row.isEmptyLine())
        {
            if (!opt.preserveEmptyLines)
                continue;

            ++emptyRun;
            if (emptyRun > std::max(0, opt.maxEmptyLines))
                continue;

            pushParagraph(out, QString(), pageIndex);
            continue;
        }

        emptyRun = 0;
        pushParagraph(out, row.text, pageIndex);
    }
}

void DocumentBuilder::appendPage_FromStep3Markers(
    const Core::VirtualPage    &vp,
    const DocumentBuildOptions &opt,
    DocumentModel              *out,
    int                         pageIndex)
{
    const Tsv::LineTable *t = vp.lineTable;
    if (!t)
        return;

    QString current;
    int currentBlock = -999999;
    int currentPar   = -999999;

    bool hasActiveParagraph = false;
    int emptyRun = 0;

    auto flushParagraphIfAny = [&]()
    {
        if (!hasActiveParagraph)
            return;

        pushParagraph(out, current, pageIndex);
        current.clear();
        hasActiveParagraph = false;
    };

    auto startNewParagraph = [&](int blockNum, int parNum)
    {
        flushParagraphIfAny();
        currentBlock = blockNum;
        currentPar   = parNum;
        hasActiveParagraph = true;
    };

    for (const auto &row : t->rows)
    {
        // 1) Empty lines
        if (row.isEmptyLine())
        {
            if (!opt.preserveEmptyLines)
            {
                flushParagraphIfAny();
                continue;
            }

            flushParagraphIfAny();

            ++emptyRun;
            if (emptyRun > std::max(0, opt.maxEmptyLines))
                continue;

            pushParagraph(out, QString(), pageIndex);
            continue;
        }

        emptyRun = 0;

        // 2) Non-empty: paragraph segmentation by block/par markers
        const int b = row.blockNum;
        const int p = row.parNum;

        if (!hasActiveParagraph)
        {
            startNewParagraph(b, p);
            current = row.text;
            continue;
        }

        // Hard paragraph break: block changed
        if (b != currentBlock)
        {
            startNewParagraph(b, p);
            current = row.text;
            continue;
        }

        // Same block:
        // - preserveLineBreaks + par changed => '\n'
        // - else join with space
        if (opt.preserveLineBreaks && p != currentPar)
        {
            current.append('\n');
            current.append(row.text);
            currentPar = p;
        }
        else
        {
            if (!current.endsWith(' ') && !row.text.startsWith(' '))
                current.append(' ');
            current.append(row.text);
            currentPar = p;
        }
    }

    flushParagraphIfAny();
}

void DocumentBuilder::pushParagraph(DocumentModel *out,
                                    const QString &text,
                                    int            pageIndex)
{
    if (!out)
        return;

    DocumentBlock b;

    // --------------------------------------------------------
    // Page index is stored in model.
    // Exporters derive page breaks from pageIndex transitions.
    // --------------------------------------------------------
    b.pageIndex = pageIndex;

    b.text = text;

    out->blocks.push_back(b);
}

} // namespace Step5
