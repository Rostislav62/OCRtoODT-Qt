// ============================================================
//  OCRtoODT — Export Controller (STEP 5.4+)
//  File: src/5_export/ExportController.h
//
//  Responsibility:
//      Export ready Step5::DocumentModel to selected format.
// ============================================================

#pragma once

#include <QString>

#include "5_document/DocumentModel.h"

class ExportController
{
public:
    static bool exportTxt(const Step5::DocumentModel &doc, const QString &outputPath, bool openAfter);

    static bool exportOdt(const Step5::DocumentModel &doc, const QString &outputPath, bool openAfter);

    static bool exportDocx(const Step5::DocumentModel &doc, const QString &outputPath, bool openAfter);

};
