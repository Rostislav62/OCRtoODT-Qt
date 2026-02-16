// ============================================================
//  OCRtoODT — STEP 5.1: DocumentModel (Data Contract)
//  File: src/5_document/DocumentModel.h
//
//  Responsibility:
//      Pure in-memory document representation produced by STEP 5.2
//      and consumed by STEP 5.4+ exporters (TXT/ODT/DOCX).
//
//  Design rules:
//      - DATA ONLY (no I/O, no UI)
//      - RAM is source of truth
//      - Deterministic, reproducible structure
// ============================================================

#pragma once

#include <QString>
#include <QVector>
#include <QDateTime>

namespace Step5 {

// ------------------------------------------------------------
// Block types (minimal, extensible)
// ------------------------------------------------------------
enum class DocumentBlockType
{
    Paragraph
};


// ------------------------------------------------------------
// One logical document block
// ------------------------------------------------------------
struct DocumentBlock
{
    DocumentBlockType type = DocumentBlockType::Paragraph;

    // --------------------------------------------------------
    // Source page index (0-based)
    //
    // Purpose:
    //     Page breaks are no longer stored as DocumentBlockType.
    //     Exporters derive page breaks by detecting pageIndex
    //     transitions (Layout-controlled).
    //
    // Meaning:
    //     0 = first OCR page, 1 = second OCR page, ...
    // --------------------------------------------------------
    int pageIndex = 0;

    // For Paragraph:
    //  - text may contain '\n' if preserveLineBreaks == true
    //  - empty string == empty paragraph (if preserved)
    QString text;
};

// ------------------------------------------------------------
// Paragraph build policy
// ------------------------------------------------------------
enum class ParagraphPolicy
{
    // Each OCR line becomes a paragraph
    MVP_LinePerParagraph = 0,

    // Use STEP 3 markers (blockNum / parNum)
    FromStep3Markers     = 1
};

// ------------------------------------------------------------
// Build options snapshot (for reproducibility / debug)
// ------------------------------------------------------------
struct DocumentBuildOptions
{
    bool pageBreak = true;

    bool preserveEmptyLines = false;
    int  maxEmptyLines      = 1;

    bool preserveLineBreaks = false;

    ParagraphPolicy paragraphPolicy =
        ParagraphPolicy::FromStep3Markers;

    // Reserved for future exporters (ODT/DOCX styles)
    QString textAlign = "justify";
};

// ------------------------------------------------------------
// Final document model
// ------------------------------------------------------------
struct DocumentModel
{
    QDateTime builtAtUtc = QDateTime::currentDateTimeUtc();
    DocumentBuildOptions options;

    QVector<DocumentBlock> blocks;

    bool isEmpty() const { return blocks.isEmpty(); }
};

} // namespace Step5
