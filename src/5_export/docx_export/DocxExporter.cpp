// ============================================================
//  OCRtoODT — DOCX Exporter (STEP 5.6)
//  File: src/5_export/docx_export/DocxExporter.cpp
//
//  Responsibility:
//      Export Step5::DocumentModel to DOCX file.
//
//  Extended from MVP:
//      - paragraphs
//      - page breaks
//      - UTF-8
//      - layout-driven formatting (font, alignment, margins,
//        indent, spacing, line height)
//      - structural normalization (max empty lines)
//      - no Qt private API
//
//  Architecture (DOCX):
//      - document.xml contains structure only (paragraphs, page breaks)
//      - styles.xml contains formatting (Normal paragraph style)
//      - [Content_Types].xml declares both document.xml and styles.xml
//      - _rels/.rels binds package → word/document.xml
// ============================================================

#include "5_export/docx_export/DocxExporter.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextStream>
#include <QStringConverter>

#include "core/LogRouter.h"
#include "5_export/ExportTextNormalizer.h"

namespace {

// ------------------------------------------------------------
// XML helpers
// ------------------------------------------------------------
QString xmlEscape(const QString &s)
{
    QString out;
    out.reserve(s.size());

    for (QChar c : s)
    {
        switch (c.unicode())
        {
        case '&':  out += "&amp;";  break;
        case '<':  out += "&lt;";   break;
        case '>':  out += "&gt;";   break;
        case '"':  out += "&quot;"; break;
        case '\'': out += "&apos;"; break;
        default:   out += c;        break;
        }
    }
    return out;
}

// ------------------------------------------------------------
// Unit conversion helpers (Layout → DOCX)
// ------------------------------------------------------------

// 1 point = 20 twips
static int ptToTwips(double pt)
{
    return static_cast<int>(pt * 20.0);
}

// 1 mm ≈ 56.7 twips
static int mmToTwips(double mm)
{
    return static_cast<int>(mm * 56.7);
}

// Font size in DOCX = half-points
static int ptToHalfPoints(int pt)
{
    return pt * 2;
}

// ------------------------------------------------------------
// Alignment mapping
// ------------------------------------------------------------
static QString alignmentToDocx(Qt::Alignment a)
{
    if (a == Qt::AlignLeft)
        return "left";

    if (a == Qt::AlignCenter)
        return "center";

    if (a == Qt::AlignRight)
        return "right";

    return "both"; // justify
}

// ============================================================
// DOCX Style Factory
// ============================================================
//
// Goal:
//   Generate WordprocessingML styles.xml from OdtLayoutModel.
//   Avoid per-paragraph formatting duplication.
//
// Notes:
//   - Keep it minimal but correct.
//   - Use a single layout-driven "Normal" paragraph style.
//   - You can extend later with more styles (Heading1, etc.).
// ============================================================

class DocxStyleFactory
{
public:
    // --------------------------------------------------------
    // Build word/styles.xml
    // --------------------------------------------------------
    static QString buildStylesXml(const OdtLayoutModel &layout)
    {
        // ----------------------------------------------------
        // DOCX "line" spacing is in twips and depends on:
        //   w:line      = (fontSizePt * lineHeightPercent / 100) in pt → twips
        //   w:lineRule  = "auto"
        //
        // This matches typical Word behavior for "Single/1.5/Double"
        // when using auto rule.
        // ----------------------------------------------------
        const double linePt =
            static_cast<double>(layout.fontSizePt()) *
            static_cast<double>(layout.lineHeightPercent()) / 100.0;

        const int lineTwips = ptToTwips(linePt);

        QString xml;
        QTextStream out(&xml);

        out <<
            R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:styles xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">

  <!-- =======================================================
       Default paragraph style: Normal
       Layout-driven formatting is defined here.
       ======================================================= -->
  <w:style w:type="paragraph" w:default="1" w:styleId="Normal">
    <w:name w:val="Normal"/>

    <!-- Paragraph properties -->
    <w:pPr>
      <w:jc w:val=")" << alignmentToDocx(layout.alignment()) << R"("/>
      <w:ind w:firstLine=")" << mmToTwips(layout.firstLineIndentMM()) << R"("/>
      <w:spacing
          w:after=")" << ptToTwips(layout.paragraphSpacingAfterPt()) << R"("
          w:line=")"  << lineTwips << R"("
          w:lineRule="auto"/>
    </w:pPr>

    <!-- Run properties -->
    <w:rPr>
      <w:rFonts
          w:ascii=")" << layout.fontName() << R"("
          w:hAnsi=")" << layout.fontName() << R"("/>
      <w:sz w:val=")" << ptToHalfPoints(layout.fontSizePt()) << R"("/>
    </w:rPr>
  </w:style>

</w:styles>
)";

        return xml;
    }
};

// ------------------------------------------------------------
// Write [Content_Types].xml
// ------------------------------------------------------------
bool writeContentTypes(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    out <<
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml"  ContentType="application/xml"/>
  <Override PartName="/word/document.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.document.main+xml"/>
  <Override PartName="/word/styles.xml"
            ContentType="application/vnd.openxmlformats-officedocument.wordprocessingml.styles+xml"/>
</Types>)";

    return true;
}

// ------------------------------------------------------------
// Write _rels/.rels
// ------------------------------------------------------------
bool writeRels(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    out <<
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1"
                Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument"
                Target="word/document.xml"/>
</Relationships>)";

    return true;
}

// ------------------------------------------------------------
// Write word/styles.xml
// ------------------------------------------------------------
bool writeStylesXml(const OdtLayoutModel &layout,
                    const QString        &path)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    // --------------------------------------------------------
    // Layout-driven styles.xml generated by DocxStyleFactory.
    // --------------------------------------------------------
    out << DocxStyleFactory::buildStylesXml(layout);

    return true;
}

// ------------------------------------------------------------
// Write word/document.xml
// ------------------------------------------------------------
bool writeDocumentXml(const Step5::DocumentModel &doc,
                      const OdtLayoutModel       &layout,
                      const QString              &path)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    out <<
        R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">
<w:body>
)";

    // --------------------------------------------------------
    // Structural normalization (max empty lines)
    // --------------------------------------------------------
    Step5::DocumentModel normalized =
        Export::ExportTextNormalizer::normalize(
            doc,
            layout.maxEmptyLines());

    int lastPageIndex = -1;

    for (const auto &block : normalized.blocks)
    {
        // ----------------------------------------------------
        // Page break between OCR pages (Layout-controlled)
        // ----------------------------------------------------
        if (layout.pageBreakEnabled())
        {
            if (lastPageIndex != -1 &&
                block.pageIndex != lastPageIndex)
            {
                out <<
                    R"(  <w:p>
    <w:r>
      <w:br w:type="page"/>
    </w:r>
  </w:p>
)";
            }

            lastPageIndex = block.pageIndex;
        }

        // ----------------------------------------------------
        // Paragraph
        //   - uses Normal style from styles.xml
        //   - document.xml remains structural only
        // ----------------------------------------------------
        const QString text = xmlEscape(block.text);

        out <<
            R"(  <w:p>
    <w:pPr>
      <w:pStyle w:val="Normal"/>
    </w:pPr>
    <w:r>
      <w:t xml:space="preserve">)" << text << R"(</w:t>
    </w:r>
  </w:p>
)";
    }

    // --------------------------------------------------------
    // Section properties (page margins)
    // --------------------------------------------------------
    out <<
        "  <w:sectPr>\n"
        "    <w:pgMar "
        << "w:top=\""    << mmToTwips(layout.marginTopMM())    << "\" "
        << "w:bottom=\"" << mmToTwips(layout.marginBottomMM()) << "\" "
        << "w:left=\""   << mmToTwips(layout.marginLeftMM())   << "\" "
        << "w:right=\""  << mmToTwips(layout.marginRightMM())  << "\"/>\n"
                                                                "  </w:sectPr>\n";

    out <<
        R"(</w:body>
</w:document>)";

    return true;
}

} // anonymous namespace

// ============================================================
// Public API
// ============================================================

namespace Export {

bool DocxExporter::writeDocxFile(
    const Step5::DocumentModel &document,
    const OdtLayoutModel       &layout,
    const QString              &outputPath)
{
    if (document.isEmpty())
    {
        LogRouter::instance().warning(
            "[DocxExporter] Document is empty — nothing to export");
        return false;
    }

    // --------------------------------------------------------
    // Prepare temp directory
    // --------------------------------------------------------
    QTemporaryDir tempDir;
    if (!tempDir.isValid())
    {
        LogRouter::instance().warning(
            "[DocxExporter] Failed to create temp directory");
        return false;
    }

    const QString root = tempDir.path();

    QDir().mkpath(root + "/word");
    QDir().mkpath(root + "/_rels");

    // --------------------------------------------------------
    // Write XML files
    // --------------------------------------------------------
    if (!writeContentTypes(root + "/[Content_Types].xml") ||
        !writeRels(root + "/_rels/.rels") ||
        !writeStylesXml(layout, root + "/word/styles.xml") ||
        !writeDocumentXml(document, layout, root + "/word/document.xml"))
    {
        LogRouter::instance().warning(
            "[DocxExporter] Failed to write DOCX XML files");
        return false;
    }

    // --------------------------------------------------------
    // ZIP → DOCX (use system zip)
    // --------------------------------------------------------
    QFileInfo outInfo(outputPath);
    QDir outDir = outInfo.dir();
    if (!outDir.exists())
        outDir.mkpath(".");

    QProcess zip;
    zip.setWorkingDirectory(root);

    const QStringList args = {
        "-r",
        outputPath,
        "."
    };

    zip.start("zip", args);
    zip.waitForFinished(-1);

    if (zip.exitStatus() != QProcess::NormalExit ||
        zip.exitCode() != 0)
    {
        LogRouter::instance().warning(
            "[DocxExporter] zip failed to create DOCX");
        return false;
    }

    LogRouter::instance().info(
        QString("[DocxExporter] DOCX written: %1").arg(outputPath));

    return true;
}

} // namespace Export
