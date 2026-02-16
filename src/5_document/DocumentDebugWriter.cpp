// ============================================================
//  OCRtoODT — STEP 5.3: DocumentDebugWriter
//  File: src/5_document/DocumentDebugWriter.cpp
// ============================================================

#include "5_document/DocumentDebugWriter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QStringConverter>

#include "core/LogRouter.h"

namespace Step5 {

void DocumentDebugWriter::writeIfEnabled(const DocumentModel &doc,
                                         const bool debugEnabled)
{
    // STRICT: no files, no debug logs when disabled
    if (!debugEnabled)
        return;

    const QString dir = debugDir();
    if (!ensureDir(dir))
        return;

    const QString j = jsonPath();
    const QString t = txtPath();

    const bool okJson = writeJson(doc, j);
    const bool okTxt  = writeTxt (doc, t);

    // Debug logs allowed only when enabled
    if (okJson && okTxt)
    {
        LogRouter::instance().info(
            QString("[STEP5][DEBUG] Document debug written: %1").arg(dir));
    }
    else
    {
        LogRouter::instance().warning(
            QString("[STEP5][DEBUG] Document debug write failed: %1").arg(dir));
    }
}

QString DocumentDebugWriter::debugDir()
{
    return "cache/document";
}

QString DocumentDebugWriter::jsonPath()
{
    return QDir::cleanPath(debugDir() + "/document.json");
}

QString DocumentDebugWriter::txtPath()
{
    return QDir::cleanPath(debugDir() + "/document.txt");
}

bool DocumentDebugWriter::ensureDir(const QString &path)
{
    QDir d;
    return d.mkpath(path);
}

bool DocumentDebugWriter::writeJson(const DocumentModel &doc, const QString &path)
{
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    out << "{\n";
    out << "  \"version\": 1,\n";
    out << "  \"builtAtUtc\": \"" << doc.builtAtUtc.toString(Qt::ISODate) << "\",\n";
    out << "  \"options\": {\n";
    out << "    \"pageBreak\": " << (doc.options.pageBreak ? "true" : "false") << ",\n";
    out << "    \"preserveEmptyLines\": " << (doc.options.preserveEmptyLines ? "true" : "false") << ",\n";
    out << "    \"maxEmptyLines\": " << doc.options.maxEmptyLines << ",\n";
    out << "    \"preserveLineBreaks\": " << (doc.options.preserveLineBreaks ? "true" : "false") << ",\n";
    out << "    \"paragraphPolicy\": " << static_cast<int>(doc.options.paragraphPolicy) << ",\n";
    out << "    \"textAlign\": \"" << escapeJson(doc.options.textAlign) << "\"\n";
    out << "  },\n";
    out << "  \"blocks\": [\n";

    for (int i = 0; i < doc.blocks.size(); ++i)
    {
        const auto &b = doc.blocks[i];
        out << "    { ";
        out << "\"type\": " << static_cast<int>(b.type) << ", ";
        out << "\"text\": \"" << escapeJson(b.text) << "\" }";
        if (i + 1 < doc.blocks.size())
            out << ",";
        out << "\n";
    }

    out << "  ]\n";
    out << "}\n";

    return true;
}

bool DocumentDebugWriter::writeTxt(const DocumentModel &doc, const QString &path)
{
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    int lastPageIndex = -1;

    for (const auto &b : doc.blocks)
    {
        // --------------------------------------------------------
        // Page breaks are no longer stored as blocks.
        // Debug output marks page transitions using pageIndex.
        // --------------------------------------------------------
        static int lastPageIndex = -1;

        if (lastPageIndex != -1 && b.pageIndex != lastPageIndex)
        {
            out << "\n\n=== PAGE BREAK (pageIndex transition) ===\n\n";
        }

        lastPageIndex = b.pageIndex;


        out << b.text;
        out << "\n\n";
    }

    return true;
}

QString DocumentDebugWriter::escapeJson(const QString &s)
{
    QString out;
    out.reserve(s.size());

    for (QChar c : s)
    {
        switch (c.unicode())
        {
        case '\\': out += "\\\\"; break;
        case '\"': out += "\\\""; break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
            out += c;
        }
    }

    return out;
}

} // namespace Step5
