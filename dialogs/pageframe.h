// ============================================================
//  OCRtoODT — PageFrame Widget
//  File: dialogs/pageframe.h
//
//  Responsibility:
//      - Draw a paper-like page preview in the ODT settings tab
//      - Visualize page margins and formatted preview text
//      - Use OdtLayoutModel as a single layout source
//
//  Notes:
//      - Pure rendering widget
//      - Contains NO configuration logic
//      - Receives fully prepared layout model
// ============================================================

#ifndef PAGEFRAME_H
#define PAGEFRAME_H

#include <QLabel>
#include <QRect>
#include "core/layout/OdtLayoutModel.h"

enum class PaperFormat {
    A4,
    Letter,
    Legal
};

class PageFrame : public QLabel
{
    Q_OBJECT

public:
    explicit PageFrame(QWidget *parent = nullptr);
    ~PageFrame() override = default;

    // ------------------------------------------------------------
    // Layout Model
    // ------------------------------------------------------------
    void setLayoutModel(const OdtLayoutModel &model);

    // ------------------------------------------------------------
    // Paper format
    // ------------------------------------------------------------
    void setPaperFormat(PaperFormat fmt);

    // ------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------
    QRect innerContentRect() const { return m_innerRect; }
    double scaleMMtoPX() const { return m_scale; }

signals:
    void layoutChanged(const QRect &innerRect, double mmToPxScale);

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QSizeF paperSizeMM() const;

    // ------------------------------------------------------------
    // Layout model (single source of truth)
    // ------------------------------------------------------------
    OdtLayoutModel m_model;

    // ------------------------------------------------------------
    // Paper state
    // ------------------------------------------------------------
    PaperFormat m_format = PaperFormat::A4;
    QRect m_innerRect;
    double m_scale = 1.0;
    bool m_hovered = false;

    // ------------------------------------------------------------
    // Preview content
    // ------------------------------------------------------------
    QString m_previewTitle;
    QString m_previewText;
};

#endif // PAGEFRAME_H
