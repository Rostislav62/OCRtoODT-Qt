// ============================================================
//  OCRtoODT — ODT Layout Model
//  File: src/core/layout/OdtLayoutModel.h
//
//  Responsibility:
//      Immutable formatting model for ODT document layout.
//      Reads all layout-related parameters from ConfigManager
//      and exposes them in normalized, strongly-typed form.
//
//  Used by:
//      - PageFrame (preview)
//      - OdtExporter (document generation)
//
//  This class:
//      • is NOT a singleton
//      • does NOT modify config
//      • does NOT depend on UI
// ============================================================

#pragma once

#include <QString>
#include <Qt>

class OdtLayoutModel
{
public:
    OdtLayoutModel();

    // Load parameters from ConfigManager
    void loadFromConfig();

    // --------------------------------------------------------
    // Typography
    // --------------------------------------------------------
    QString fontName() const;
    int     fontSizePt() const;
    Qt::Alignment alignment() const;

    double  firstLineIndentMM() const;
    double  paragraphSpacingAfterPt() const;
    double  lineHeightPercent() const;

    // --------------------------------------------------------
    // Page layout
    // --------------------------------------------------------
    double  marginLeftMM() const;
    double  marginRightMM() const;
    double  marginTopMM() const;
    double  marginBottomMM() const;

    bool    pageBreakEnabled() const;

    // --------------------------------------------------------
    // Structural normalization
    // --------------------------------------------------------
    int     maxEmptyLines() const;

    // --------------------------------------------------------
    // Setters (used by UI preview)
    // --------------------------------------------------------
    void setFontName(const QString &name);
    void setFontSizePt(int pt);
    void setAlignment(Qt::Alignment align);

    void setFirstLineIndentMM(double mm);
    void setParagraphSpacingAfterPt(double pt);
    void setLineHeightPercent(double percent);

    void setMarginLeftMM(double mm);
    void setMarginRightMM(double mm);
    void setMarginTopMM(double mm);
    void setMarginBottomMM(double mm);

    void setPageBreakEnabled(bool enabled);

    void setMaxEmptyLines(int value);

    QString paperSizeKey() const;


private:
    // Typography
    QString       m_fontName;
    int           m_fontSizePt = 12;
    Qt::Alignment m_alignment  = Qt::AlignJustify;

    double  m_firstLineIndentMM        = 10.0;
    double  m_paragraphSpacingAfterPt  = 6.0;
    double  m_lineHeightPercent        = 120.0;

    // Page layout
    double  m_marginLeftMM   = 20.0;
    double  m_marginRightMM  = 15.0;
    double  m_marginTopMM    = 20.0;
    double  m_marginBottomMM = 15.0;

    bool    m_pageBreak = true;

    // Structural
    int     m_maxEmptyLines = 1;

    QString m_paperSizeKey = "A4";


    Qt::Alignment parseAlignment(const QString &value) const;
};
