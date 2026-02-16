// ============================================================
//  OCRtoODT — TXT Exporter
//  File: src/5_export/txt_export/TxtExporter.cpp
//
//  Responsibility:
//      Serialize Step5::DocumentModel into plain UTF-8 TXT.
//
//  Rules:
//      - Paragraph → text + blank line
//      - PageBreak → two blank lines
//      - UTF-8 always
// ============================================================



#include <QFile>
#include <QTextStream>
#include <QStringConverter>

#include "core/LogRouter.h"
#include "5_export/txt_export/TxtExporter.h"
#include "5_export/ExportTextNormalizer.h"
#include "core/layout/OdtLayoutModel.h"


namespace Export {

bool TxtExporter::writeTxtFile(
    const Step5::DocumentModel &document,
    const OdtLayoutModel       &layout,
    const QString              &outputPath)


{
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        LogRouter::instance().warning(
            QString("[TxtExporter] Cannot open file: %1").arg(outputPath));
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // --------------------------------------------------------
    // Structural normalization (max empty lines)
    // --------------------------------------------------------
    Step5::DocumentModel normalized =
        ExportTextNormalizer::normalize(document, layout.maxEmptyLines());

    int lastPageIndex = -1;

    for (const auto &block : document.blocks)
    {
        // --------------------------------------------------------
        // Page break between OCR pages (Layout-controlled)
        // --------------------------------------------------------
        if (layout.pageBreakEnabled())
        {
            if (lastPageIndex != -1 &&
                block.pageIndex != lastPageIndex)
            {
                // Two blank lines as a stable TXT page delimiter
                out << "\n\n";
            }

            lastPageIndex = block.pageIndex;
        }

        // --------------------------------------------------------
        // Paragraph
        // --------------------------------------------------------
        out << block.text << "\n\n";
    }


    return true;
}

} // namespace Export
