// ============================================================
//  OCRtoODT — ODT Layout Model
//  File: src/core/layout/OdtLayoutModel.cpp
// ============================================================

#include "OdtLayoutModel.h"

#include "core/ConfigManager.h"


// ============================================================
//  Constructor
// ============================================================

OdtLayoutModel::OdtLayoutModel()
{
}


// ============================================================
//  Load from ConfigManager
// ============================================================

void OdtLayoutModel::loadFromConfig()
{
    ConfigManager &cfg = ConfigManager::instance();

    // --------------------------------------------------------
    // Typography
    // --------------------------------------------------------
    m_fontName = cfg.get("odt.font_name", "Times New Roman").toString();
    m_fontSizePt = cfg.get("odt.font_size_pt", 12).toInt();

    m_alignment = parseAlignment(
        cfg.get("odt.text_align", "justify").toString()
        );

    m_firstLineIndentMM =
        cfg.get("odt.first_line_indent_mm", 10).toDouble();

    m_paragraphSpacingAfterPt =
        cfg.get("odt.paragraph_spacing_after_pt", 6).toDouble();

    m_lineHeightPercent =
        cfg.get("odt.line_height_percent", 120).toDouble();

    // --------------------------------------------------------
    // Page layout
    // --------------------------------------------------------
    m_marginLeftMM =
        cfg.get("odt.margin_left_mm", 20).toDouble();

    m_marginRightMM =
        cfg.get("odt.margin_right_mm", 15).toDouble();

    m_marginTopMM =
        cfg.get("odt.margin_top_mm", 20).toDouble();

    m_marginBottomMM =
        cfg.get("odt.margin_bottom_mm", 15).toDouble();

    m_pageBreak =
        cfg.get("odt.page_break", true).toBool();

    m_paperSizeKey =
        cfg.get("odt.paper_size", "A4").toString();


    // --------------------------------------------------------
    // Structural normalization
    // --------------------------------------------------------
    m_maxEmptyLines =
        cfg.get("odt.max_empty_lines", 1).toInt();

    if (m_maxEmptyLines < 0)
        m_maxEmptyLines = 0;
    if (m_maxEmptyLines > 3)
        m_maxEmptyLines = 3;


}


// ============================================================
//  Alignment parsing
// ============================================================

Qt::Alignment OdtLayoutModel::parseAlignment(const QString &value) const
{
    if (value == "left")
        return Qt::AlignLeft;

    if (value == "center")
        return Qt::AlignCenter;

    if (value == "right")
        return Qt::AlignRight;

    return Qt::AlignJustify;
}


// ============================================================
//  Getters
// ============================================================

QString OdtLayoutModel::fontName() const { return m_fontName; }
int     OdtLayoutModel::fontSizePt() const { return m_fontSizePt; }
Qt::Alignment OdtLayoutModel::alignment() const { return m_alignment; }

double OdtLayoutModel::firstLineIndentMM() const { return m_firstLineIndentMM; }
double OdtLayoutModel::paragraphSpacingAfterPt() const { return m_paragraphSpacingAfterPt; }
double OdtLayoutModel::lineHeightPercent() const { return m_lineHeightPercent; }

double OdtLayoutModel::marginLeftMM() const { return m_marginLeftMM; }
double OdtLayoutModel::marginRightMM() const { return m_marginRightMM; }
double OdtLayoutModel::marginTopMM() const { return m_marginTopMM; }
double OdtLayoutModel::marginBottomMM() const { return m_marginBottomMM; }

bool OdtLayoutModel::pageBreakEnabled() const { return m_pageBreak; }

int OdtLayoutModel::maxEmptyLines() const { return m_maxEmptyLines; }



// ============================================================
//  Setters (used by UI preview)
// ============================================================

void OdtLayoutModel::setFontName(const QString &name) { m_fontName = name; }
void OdtLayoutModel::setFontSizePt(int pt) { m_fontSizePt = pt; }
void OdtLayoutModel::setAlignment(Qt::Alignment align) { m_alignment = align; }

void OdtLayoutModel::setFirstLineIndentMM(double mm) { m_firstLineIndentMM = mm; }
void OdtLayoutModel::setParagraphSpacingAfterPt(double pt) { m_paragraphSpacingAfterPt = pt; }
void OdtLayoutModel::setLineHeightPercent(double percent) { m_lineHeightPercent = percent; }

void OdtLayoutModel::setMarginLeftMM(double mm) { m_marginLeftMM = mm; }
void OdtLayoutModel::setMarginRightMM(double mm) { m_marginRightMM = mm; }
void OdtLayoutModel::setMarginTopMM(double mm) { m_marginTopMM = mm; }
void OdtLayoutModel::setMarginBottomMM(double mm) { m_marginBottomMM = mm; }

void OdtLayoutModel::setPageBreakEnabled(bool enabled) { m_pageBreak = enabled; }

void OdtLayoutModel::setMaxEmptyLines(int value)
{
    m_maxEmptyLines = value;

    if (m_maxEmptyLines < 0)
        m_maxEmptyLines = 0;
    if (m_maxEmptyLines > 3)
        m_maxEmptyLines = 3;
}

QString OdtLayoutModel::paperSizeKey() const
{
    return m_paperSizeKey;
}

