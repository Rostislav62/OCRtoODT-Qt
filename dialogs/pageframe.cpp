// ============================================================
//  OCRtoODT — PageFrame Widget
//  File: dialogs/pageframe.cpp
//
//  Responsibility:
//      - Render page preview
//      - Use OdtLayoutModel for typography & layout
//      - Guarantee 1:1 visual behavior with exporter logic
// ============================================================

#include "pageframe.h"

#include <QPainter>
#include <QPaintEvent>
#include <QTextLayout>
#include <QTextLine>
#include <QTextBlockFormat>

// ============================================================
// Constructor
// ============================================================

PageFrame::PageFrame(QWidget *parent)
    : QLabel(parent)
{
    setAttribute(Qt::WA_Hover, true);
    setMouseTracking(true);
    setStyleSheet("background: transparent;");

    // Default preview content
    m_previewTitle = "The Future We Agreed On";

    m_previewText =
        "I often think that a successful life does not always look like a celebration. "
        "Sometimes it looks like a quiet morning, when the house is still half asleep, "
        "the light outside is soft, and the world seems to pause—just long enough for you to notice it.\n\n"
        "I love tea.\n\n"
        "It teaches patience and reminds me that some things cannot be rushed without losing their essence."
        "In my youth, I was impulsive. "
        "Psychologists would probably call me a choleric."
        "I wanted everything at once: knowledge, success, recognition, experiences. "
        "I read a lot—greedily, chaotically. "
        "I wanted to be everywhere, to know everything, to keep up with everything. "
        "By the age of thirty, I had read around ten thousand technical journals and "
        "several hundred works of fiction."
        "I wanted to play football, play table tennis, dance, "
        "read technical journals and bring what I read to life, "
        "while also reading literature and watching films. "
        "I dreamed that I had many bodies, and that I could be in all these places at once, "
        "doing all these things simultaneously. "
        "I have a very good memory—perhaps because I read so much.";
}

// ============================================================
// Layout model setter
// ============================================================

void PageFrame::setLayoutModel(const OdtLayoutModel &model)
{
    m_model = model;
    update();
}

// ============================================================
// Paper format
// ============================================================

void PageFrame::setPaperFormat(PaperFormat fmt)
{
    m_format = fmt;
    update();
}

// ============================================================
// Paper size (mm)
// ============================================================

QSizeF PageFrame::paperSizeMM() const
{
    switch (m_format) {
    case PaperFormat::A4:    return QSizeF(210.0, 297.0);
    case PaperFormat::Letter:return QSizeF(215.9, 279.4);
    case PaperFormat::Legal: return QSizeF(215.9, 355.6);
    }
    return QSizeF(210.0, 297.0);
}

// ============================================================
// Hover events
// ============================================================

void PageFrame::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    update();
    QLabel::enterEvent(event);
}

void PageFrame::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QLabel::leaveEvent(event);
}

// ============================================================
// Main rendering
// ============================================================

void PageFrame::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    QRectF widgetRect = rect();

    QSizeF paperMM = paperSizeMM();
    const double ratio = paperMM.width() / paperMM.height();

    double targetW = widgetRect.width() * 0.80;
    double targetH = targetW / ratio;

    if (targetH > widgetRect.height() * 0.85) {
        targetH = widgetRect.height() * 0.85;
        targetW = targetH * ratio;
    }

    const double x0 = (widgetRect.width()  - targetW) / 2.0;
    const double y0 = (widgetRect.height() - targetH) / 2.0;

    QRectF paperRect(x0, y0, targetW, targetH);

    // mm → px scale
    m_scale = targetW / paperMM.width();

    // Convert margins from model (mm → px)
    const double leftPx   = m_model.marginLeftMM()   * m_scale;
    const double rightPx  = m_model.marginRightMM()  * m_scale;
    const double topPx    = m_model.marginTopMM()    * m_scale;
    const double bottomPx = m_model.marginBottomMM() * m_scale;

    m_innerRect = QRect(
        static_cast<int>(paperRect.left() + leftPx),
        static_cast<int>(paperRect.top()  + topPx),
        static_cast<int>(paperRect.width()  - leftPx - rightPx),
        static_cast<int>(paperRect.height() - topPx  - bottomPx)
        );

    emit layoutChanged(m_innerRect, m_scale);

    // Shadow
    QRectF shadow = paperRect.translated(4, 4);
    p.setBrush(QColor(0, 0, 0, 40));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(shadow, 4, 4);

    // Paper
    p.setBrush(QColor("#fafafa"));
    p.setPen(QPen(QColor("#d0d0d0"), 1));
    p.drawRoundedRect(paperRect, 4, 4);

    if (m_hovered) {
        p.setPen(QPen(QColor("#3a7afe"), 2));
        p.drawRoundedRect(paperRect.adjusted(1, 1, -1, -1), 4, 4);
    }

    // Visual margin box
    p.setPen(QPen(QColor("#87a8d8"), 1, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    p.drawRect(m_innerRect);

    // ========================================================
    // Text rendering
    // ========================================================

    p.save();

    QRect textRect = m_innerRect.adjusted(12, 12, -12, -12);
    p.setClipRect(textRect);
    p.setPen(Qt::black);

    // Base font
    QFont baseFont(m_model.fontName(), m_model.fontSizePt());
    QFontMetrics fm(baseFont);

    int y = textRect.top();

    // ========================================================
    // Title (always centered)
    // ========================================================

    QFont titleFont = baseFont;
    titleFont.setBold(true);
    titleFont.setPointSize(baseFont.pointSize() + 4);

    QTextOption titleOption;
    titleOption.setAlignment(Qt::AlignCenter);
    titleOption.setWrapMode(QTextOption::WordWrap);

    QTextLayout titleLayout(m_previewTitle, titleFont);
    titleLayout.setTextOption(titleOption);

    titleLayout.beginLayout();
    while (true)
    {
        QTextLine line = titleLayout.createLine();
        if (!line.isValid())
            break;

        line.setLineWidth(textRect.width());
        line.setPosition(QPointF(textRect.left(), y));
        y += static_cast<int>(line.height());
    }
    titleLayout.endLayout();
    titleLayout.draw(&p, QPointF(0, 0));

    y += fm.height();

    // ========================================================
    // Body paragraphs
    // ========================================================

    QTextOption bodyOption;
    bodyOption.setAlignment(m_model.alignment());
    bodyOption.setWrapMode(QTextOption::WordWrap);

    const double indentPx =
        m_model.firstLineIndentMM() * m_scale;

    const int paragraphSpacingPx =
        static_cast<int>((m_model.paragraphSpacingAfterPt() / 72.0) * 96.0);

    const double lineHeightFactor =
        m_model.lineHeightPercent() / 100.0;

    QStringList paragraphs = m_previewText.split("\n\n");

    for (const QString &para : paragraphs)
    {
        QTextLayout layout(para, baseFont);
        layout.setTextOption(bodyOption);

        layout.beginLayout();

        bool firstLine = true;

        while (true)
        {
            QTextLine line = layout.createLine();
            if (!line.isValid())
                break;

            if (firstLine)
                line.setLineWidth(textRect.width() - indentPx);
            else
                line.setLineWidth(textRect.width());

            qreal x = textRect.left();
            if (firstLine)
                x += indentPx;

            line.setPosition(QPointF(x, y));

            y += static_cast<int>(line.height() * lineHeightFactor);

            firstLine = false;
        }

        layout.endLayout();
        layout.draw(&p, QPointF(0, 0));

        y += paragraphSpacingPx;
    }

    p.restore();
}
