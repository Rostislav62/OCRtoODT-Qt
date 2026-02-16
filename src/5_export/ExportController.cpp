// ============================================================
//  OCRtoODT — Export Controller (STEP 5.4+)
//  File: src/5_export/ExportController.cpp
//
//  Responsibility:
//      Export ready Step5::DocumentModel to selected format.
//
//  Architecture:
//      - Creates OdtLayoutModel
//      - Loads layout from ConfigManager
//      - Passes layout to Exporters
//
//  Important:
//      Exporters MUST NOT read ConfigManager directly.
//      All layout logic is centralized via OdtLayoutModel.
// ============================================================

#include "5_export/ExportController.h"

#include <QDesktopServices>
#include <QUrl>

#include "core/LogRouter.h"
#include "core/layout/OdtLayoutModel.h"
#include "5_export/txt_export/TxtExporter.h"
#include "5_export/odt_export/OdtExporter.h"
#include "5_export/docx_export/DocxExporter.h"


// ============================================================
//  TXT Export
// ============================================================

bool ExportController::exportTxt(
    const Step5::DocumentModel &doc,
    const QString              &outputPath,
    bool                        openAfter)
{
    LogRouter::instance().info(
        QString("[ExportController] Export TXT: %1").arg(outputPath));

    // --------------------------------------------------------
    // Create layout model (centralized formatting source)
    // --------------------------------------------------------
    OdtLayoutModel layout;
    layout.loadFromConfig();

    // --------------------------------------------------------
    // Delegate to TxtExporter with layout
    // --------------------------------------------------------
    const bool ok =
        Export::TxtExporter::writeTxtFile(doc, layout, outputPath);

    if (!ok)
    {
        LogRouter::instance().warning(
            "[ExportController] TXT export failed");
        return false;
    }

    // --------------------------------------------------------
    // Open after export (if requested)
    // --------------------------------------------------------
    if (openAfter)
        QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath));

    return true;
}


// ============================================================
//  ODT Export
// ============================================================

bool ExportController::exportOdt(
    const Step5::DocumentModel &doc,
    const QString              &outputPath,
    bool                        openAfter)
{
    LogRouter::instance().info(
        QString("[ExportController] Export ODT: %1").arg(outputPath));

    // --------------------------------------------------------
    // Create layout model
    // --------------------------------------------------------
    OdtLayoutModel layout;
    layout.loadFromConfig();

    // --------------------------------------------------------
    // Delegate to OdtExporter
    // --------------------------------------------------------
    const bool ok =
        Export::OdtExporter::writeOdtFile(doc, layout, outputPath);

    if (!ok)
    {
        LogRouter::instance().warning(
            "[ExportController] ODT export failed");
        return false;
    }

    if (openAfter)
        QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath));

    return true;
}


// ============================================================
//  DOCX Export
// ============================================================

bool ExportController::exportDocx(
    const Step5::DocumentModel &doc,
    const QString              &outputPath,
    bool                        openAfter)
{
    LogRouter::instance().info(
        QString("[ExportController] Export DOCX: %1").arg(outputPath));

    // --------------------------------------------------------
    // Create layout model
    // --------------------------------------------------------
    OdtLayoutModel layout;
    layout.loadFromConfig();

    // --------------------------------------------------------
    // Delegate to DocxExporter with layout
    // --------------------------------------------------------
    const bool ok =
        Export::DocxExporter::writeDocxFile(doc, layout, outputPath);

    if (!ok)
    {
        LogRouter::instance().warning(
            "[ExportController] DOCX export failed");
        return false;
    }

    if (openAfter)
        QDesktopServices::openUrl(QUrl::fromLocalFile(outputPath));

    return true;
}
