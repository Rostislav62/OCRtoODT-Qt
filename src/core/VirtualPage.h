// ============================================================
//  OCRtoODT — Virtual Page Representation
//  File: core/virtualpage.h
//
//  Responsibility:
//      Unified in-memory representation of a single document page.
//
//      VirtualPage is a PURE DATA OBJECT.
//      It contains NO logic and NO ownership rules.
//
//      It is the ONLY structure passed between pipeline steps.
//
//  PIPELINE CONTRACT (CRITICAL):
//      • VirtualPage is the ONLY data carrier between STEP 0 → STEP N
//      • No other temporary or parallel page representations are allowed
//      • Every STEP must respect ownership and write permissions defined below
//
//  STEP OWNERSHIP SUMMARY:
//      STEP 0 (Input):
//          - fills identification, type flags, image / PDF metadata
//
//      STEP 1 (Preprocess):
//          - may adjust image-related metadata (size, DPI)
//          - MUST NOT touch OCR-related fields
//
//      STEP 2 (OCR):
//          - fills OCR execution status and RAW OCR data
//          - MUST respect RAM-first / DISK-only contract
//
//      STEP 3 (LineTable Builder):
//          - parses RAW OCR TSV into structured LineTable
//          - MUST NOT modify OCR source data
//
//      STEP 4+ (Edit / Export):
//          - READ-ONLY access to OCR and LineTable
//
// ============================================================

#ifndef VIRTUALPAGE_H
#define VIRTUALPAGE_H

#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QSize>
#include <QUuid>

namespace Tsv {
struct LineTable;   // forward declaration
}

namespace Core {

struct VirtualPage
{
    // =========================================================
    // Identification
    // =========================================================
    //
    // STEP OWNERSHIP:
    //   STEP 0 ONLY
    //
    // CONTRACT:
    //   • id is generated once and never changes
    //   • globalIndex is stable for the entire lifetime of the project
    //
    QUuid   id = QUuid::createUuid();   // unique page id inside project
    QString displayName;                // filename or "PDF page N"
    QString sourcePath;                 // original source path (PDF or image)

    // Sequential index inside whole project (0-based)
    int globalIndex = -1;

    // =========================================================
    // Type flags
    // =========================================================
    //
    // STEP OWNERSHIP:
    //   STEP 0 ONLY
    //
    // CONTRACT:
    //   • isPdf == true  → pageIndex >= 0
    //   • isPdf == false → pageIndex == -1
    //
    bool isPdf     = false;   // true → page comes from PDF
    int  pageIndex = -1;      // PDF page number; image = -1

    // =========================================================
    // PDF metadata (if applicable)
    // =========================================================
    //
    // STEP OWNERSHIP:
    //   STEP 0 (initial)
    //   STEP 1 (optional refinement)
    //
    // CONTRACT:
    //   • valid ONLY if isPdf == true
    //   • values are informational, not authoritative layout data
    //
    int    pdfWidth    = 0;     // Poppler page width (points)
    int    pdfHeight   = 0;     // Poppler page height (points)
    int    pdfRotation = 0;     // rotation (degrees)
    double pdfDpi      = 0.0;   // DPI used for preview render

    // =========================================================
    // Image metadata (if applicable)
    // =========================================================
    //
    // STEP OWNERSHIP:
    //   STEP 0 (initial)
    //   STEP 1 (may adjust after preprocessing)
    //
    int     imgWidth  = 0;      // pixels
    int     imgHeight = 0;      // pixels
    QString imgFormat;          // e.g. "png", "jpg"

    // =========================================================
    // OCR RESULTS — STEP 2 (RAM-FIRST, CONTRACTUAL)
    // =========================================================
    //
    // STEP OWNERSHIP:
    //   STEP 2 ONLY
    //
    // CONTRACT (CRITICAL INVARIANTS):
    //
    //   • ocrSuccess == true  ⇒
    //         (ocrTsvText is non-empty) OR (ocrTsvPath is non-empty)
    //
    //   • ocrSuccess == false ⇒
    //         ocrTsvText MUST be empty
    //         ocrTsvPath MUST be empty
    //
    //   • ocrTsvText is the PRIMARY SOURCE OF TRUTH
    //   • ocrTsvPath is OPTIONAL and used only for:
    //         - debug_mode
    //         - disk_only execution mode
    //
    //   • No STEP except STEP 2 may modify these fields
    //

    // --- OCR execution status ---
    bool ocrSuccess = false;

    // --- RAW OCR TSV (RAM) ---
    QString ocrTsvText;

    // --- RAW OCR TSV (DISK, OPTIONAL) ---
    QString ocrTsvPath;

    // --- Parsed OCR result (RAM) ---
    //
    // STEP OWNERSHIP:
    //   STEP 3 ONLY
    //
    // OWNERSHIP RULE:
    //   • VirtualPage does NOT own this pointer
    //   • Lifetime is managed externally (STEP 3 controller)
    //
    Tsv::LineTable *lineTable = nullptr;

    // =========================================================
    // Future OCR / layout extensions (STEP 3+)
    // =========================================================
    //
    // NOTE:
    //   These fields are RESERVED.
    //   They MUST NOT be relied upon by current pipeline logic.
    //
    QString      textOcr;          // optional plain text (future)
    double       confidence = 0.0;
    QVariantList layoutBlocks;     // future structured OCR blocks

    // =========================================================
    // Helpers
    // =========================================================
    //
    // NOTE:
    //   Helpers MUST remain trivial.
    //   No logic, no validation, no side effects.
    //
    void setGlobalIndex(int idx) { globalIndex = idx; }
    int  getGlobalIndex() const  { return globalIndex; }
};

} // namespace Core

#endif // VIRTUALPAGE_H
