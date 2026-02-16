// ============================================================
//  OCRtoODT — ODT Exporter (STEP 5.5)
//  File: src/5_export/odt_export/OdtExporter.cpp
//
//  Responsibility:
//      Export Step5::DocumentModel into ODT using system `zip`.
//
//  Implementation details:
//      - Build ODT structure in QTemporaryDir
//      - Write mimetype (uncompressed, first)
//      - Write content.xml
//      - Write META-INF/manifest.xml
//      - Call `zip` via QProcess
// ============================================================



#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QStringConverter>

#include "core/LogRouter.h"
#include "core/layout/OdtLayoutModel.h"
#include "5_export/odt_export/OdtExporter.h"


namespace Export {

// ------------------------------------------------------------
// Helpers
// ------------------------------------------------------------
static bool writeFileUtf8(const QString &path, const QString &content)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << content;
    return true;
}

static QString paperSizeToFo(const QString &paperKey, bool width)
{
    // ODT expects mm
    // A4:     210 x 297
    // Letter: 215.9 x 279.4
    // Legal:  215.9 x 355.6
    if (paperKey == "Letter")
        return width ? "215.9mm" : "279.4mm";
    if (paperKey == "Legal")
        return width ? "215.9mm" : "355.6mm";
    return width ? "210mm" : "297mm";
}

static QString alignmentToFo(Qt::Alignment align)
{
    if (align == Qt::AlignLeft)   return "left";
    if (align == Qt::AlignCenter) return "center";
    if (align == Qt::AlignRight)  return "right";
    return "justify";
}

// ------------------------------------------------------------
// Build content.xml using DocumentModel + OdtLayoutModel
// ------------------------------------------------------------
static QString buildContentXml(const Step5::DocumentModel &doc,
                               const OdtLayoutModel       &layout)
{
    // --------------------------------------------------------
    // Note:
    // Page margins MUST be defined in page-layout-properties,
    // not in paragraph-properties. Otherwise LibreOffice will
    // keep default page margins.
    // --------------------------------------------------------

    const QString paperKey = layout.paperSizeKey();

    const QString pageW = paperSizeToFo(paperKey, true);
    const QString pageH = paperSizeToFo(paperKey, false);

    QString xml;
    QTextStream out(&xml);

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<office:document-content "
           "xmlns:office=\"urn:oasis:names:tc:opendocument:xmlns:office:1.0\" "
           "xmlns:text=\"urn:oasis:names:tc:opendocument:xmlns:text:1.0\" "
           "xmlns:style=\"urn:oasis:names:tc:opendocument:xmlns:style:1.0\" "
           "xmlns:fo=\"urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0\" "
           "office:version=\"1.2\">\n";

    // --------------------------------------------------------
    // Automatic styles
    // --------------------------------------------------------
    out << "  <office:automatic-styles>\n";

    // ========================================================
    // Page layout (margins + page size)
    // ========================================================
    out << "    <style:page-layout style:name=\"PM1\">\n";
    out << "      <style:page-layout-properties "
        << "fo:page-width=\""  << pageW << "\" "
        << "fo:page-height=\"" << pageH << "\" "
        << "style:print-orientation=\"portrait\" "
        << "fo:margin-top=\""    << layout.marginTopMM()    << "mm\" "
        << "fo:margin-bottom=\"" << layout.marginBottomMM() << "mm\" "
        << "fo:margin-left=\""   << layout.marginLeftMM()   << "mm\" "
        << "fo:margin-right=\""  << layout.marginRightMM()  << "mm\"/>\n";
    out << "    </style:page-layout>\n";

    // ========================================================
    // Paragraph style (font + alignment + indent + spacing + line height)
    // ========================================================
    out << "    <style:style style:name=\"P1\" style:family=\"paragraph\">\n";

    out << "      <style:text-properties "
        << "fo:font-family=\"" << layout.fontName() << "\" "
        << "fo:font-size=\""   << layout.fontSizePt() << "pt\"/>\n";

    out << "      <style:paragraph-properties "
        << "fo:text-align=\""   << alignmentToFo(layout.alignment()) << "\" "
        << "fo:text-indent=\""  << layout.firstLineIndentMM() << "mm\" "
        << "fo:margin-bottom=\"" << layout.paragraphSpacingAfterPt() << "pt\" "
        << "fo:line-height=\""  << layout.lineHeightPercent() << "%\"/>\n";

    out << "    </style:style>\n";

    // ========================================================
    // Page break style (real page break)
    // ========================================================
    out << "    <style:style style:name=\"PB\" style:family=\"paragraph\">\n";
    out << "      <style:paragraph-properties fo:break-before=\"page\"/>\n";
    out << "    </style:style>\n";

    out << "  </office:automatic-styles>\n";

    // --------------------------------------------------------
    // Master styles (bind page layout to Standard master page)
    // --------------------------------------------------------
    out << "  <office:master-styles>\n";
    out << "    <style:master-page style:name=\"Standard\" "
           "style:page-layout-name=\"PM1\"/>\n";
    out << "  </office:master-styles>\n";

    // --------------------------------------------------------
    // Body
    // --------------------------------------------------------
    out << "  <office:body>\n";
    out << "    <office:text>\n";

    int emptyLineCounter = 0;
    int lastPageIndex = -1;

    for (const auto &b : doc.blocks)
    {
        // =====================================================
        // Page break between OCR pages (Layout-controlled)
        // =====================================================
        if (layout.pageBreakEnabled())
        {
            if (lastPageIndex != -1 &&
                b.pageIndex != lastPageIndex)
            {
                // Reset empty-lines counter at real page boundary
                emptyLineCounter = 0;

                out << "      <text:p text:style-name=\"PB\"/>\n";
            }

            lastPageIndex = b.pageIndex;
        }


        // =====================================================
        // Paragraph normalization (max empty lines logic)
        // =====================================================
        const bool isEmpty = b.text.trimmed().isEmpty();

        if (isEmpty)
        {
            ++emptyLineCounter;

            if (emptyLineCounter > layout.maxEmptyLines())
                continue;
        }
        else
        {
            emptyLineCounter = 0;
        }

        // =====================================================
        // Paragraph output
        // =====================================================
        out << "      <text:p text:style-name=\"P1\">";

        const QString escaped =
            b.text
                .toHtmlEscaped()
                .replace("\n", "<text:line-break/>");

        out << escaped;
        out << "</text:p>\n";
    }

    out << "    </office:text>\n";
    out << "  </office:body>\n";
    out << "</office:document-content>\n";

    return xml;
}




static QString buildManifestXml()
{
    return QString(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<manifest:manifest "
        "xmlns:manifest=\"urn:oasis:names:tc:opendocument:xmlns:manifest:1.0\" "
        "manifest:version=\"1.2\">\n"
        "  <manifest:file-entry manifest:media-type=\"application/vnd.oasis.opendocument.text\" manifest:full-path=\"/\"/>\n"
        "  <manifest:file-entry manifest:media-type=\"text/xml\" manifest:full-path=\"content.xml\"/>\n"
        "</manifest:manifest>\n");
}

// ------------------------------------------------------------
// Public API
// ------------------------------------------------------------
bool OdtExporter::writeOdtFile(const Step5::DocumentModel &document,
                               const OdtLayoutModel       &layout,
                               const QString              &outputPath)

{
    if (document.isEmpty())
    {
        LogRouter::instance().warning(
            "[OdtExporter] Document is empty — nothing to export");
        return false;
    }

    QTemporaryDir tmp;
    if (!tmp.isValid())
    {
        LogRouter::instance().error(
            "[OdtExporter] Cannot create temporary directory");
        return false;
    }

    const QString root = tmp.path();

    // --------------------------------------------------------
    // mimetype (MUST be first, uncompressed)
    // --------------------------------------------------------
    if (!writeFileUtf8(root + "/mimetype",
                       "application/vnd.oasis.opendocument.text"))
    {
        LogRouter::instance().error("[OdtExporter] Failed to write mimetype");
        return false;
    }

    // --------------------------------------------------------
    // content.xml
    // --------------------------------------------------------
    if (!writeFileUtf8(root + "/content.xml",
                       buildContentXml(document, layout)))
    {
        LogRouter::instance().error("[OdtExporter] Failed to write content.xml");
        return false;
    }

    // --------------------------------------------------------
    // META-INF/manifest.xml
    // --------------------------------------------------------
    QDir().mkpath(root + "/META-INF");
    if (!writeFileUtf8(root + "/META-INF/manifest.xml",
                       buildManifestXml()))
    {
        LogRouter::instance().error("[OdtExporter] Failed to write manifest.xml");
        return false;
    }

    // --------------------------------------------------------
    // zip (system)
    // --------------------------------------------------------
    QProcess zip;
    zip.setWorkingDirectory(root);

    QStringList args;
    args << "-0" << "-X" << outputPath << "mimetype";
    zip.start("zip", args);
    if (!zip.waitForFinished() || zip.exitCode() != 0)
    {
        LogRouter::instance().error(
            "[OdtExporter] zip failed on mimetype");
        return false;
    }

    args.clear();
    args << "-r" << outputPath << "." << "-x" << "mimetype";
    zip.start("zip", args);
    if (!zip.waitForFinished() || zip.exitCode() != 0)
    {
        LogRouter::instance().error(
            "[OdtExporter] zip failed on content");
        return false;
    }

    LogRouter::instance().info(
        QString("[OdtExporter] ODT written: %1").arg(outputPath));

    return true;
}

} // namespace Export
