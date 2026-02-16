// ============================================================
//  OCRtoODT — STEP 5.3: DocumentDebugWriter
//  File: src/5_document/DocumentDebugWriter.h
//
//  Responsibility:
//      Serialize DocumentModel to debug artifacts on disk
//      STRICTLY when debugEnabled == true.
//
//  Rules:
//      - No-op when debug disabled
//      - Overwrite last artifacts
//      - JSON is source of truth; TXT is visual aid
// ============================================================

#pragma once

#include <QString>

#include "5_document/DocumentModel.h"

namespace Step5 {

class DocumentDebugWriter
{
public:
    // Safe to call unconditionally. No-op if debugEnabled == false.
    static void writeIfEnabled(const DocumentModel &doc, bool debugEnabled);

private:
    static QString debugDir();   // "cache/document"
    static QString jsonPath();   // "cache/document/document.json"
    static QString txtPath();    // "cache/document/document.txt"

    static bool ensureDir(const QString &path);

    static bool writeJson(const DocumentModel &doc, const QString &path);
    static bool writeTxt (const DocumentModel &doc, const QString &path);

    static QString escapeJson(const QString &s);
};

} // namespace Step5
