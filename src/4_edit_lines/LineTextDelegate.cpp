// ============================================================
//  OCRtoODT — STEP 4: Line Text Delegate
//  File: src/4_edit_lines/LineTextDelegate.cpp
//
//  Responsibility:
//      Custom item delegate for OCR line rendering and editing.
// ============================================================

#include "4_edit_lines/LineTextDelegate.h"

#include <QPainter>
#include <QStyleOptionViewItem>
#include <QTextOption>

namespace Step4 {

LineTextDelegate::LineTextDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

QSize LineTextDelegate::sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    const QString text = index.data(Qt::DisplayRole).toString();

    QFontMetrics fm(option.font);

    // Calculate multi-line height based on available width
    int width = option.widget
                    ? option.widget->width() - 12
                    : 400;

    QRect br = fm.boundingRect(
        QRect(0, 0, width, 10000),
        Qt::TextWordWrap,
        text);

    int h = br.height() + 8;   // padding

    // Minimal comfortable height
    if (h < fm.height() + 8)
        h = fm.height() + 8;

    return QSize(width, h);
}

void LineTextDelegate::paint(QPainter *painter,
                             const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    painter->save();

    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());

    const QString text = index.data(Qt::DisplayRole).toString();

    painter->setPen(option.palette.text().color());

    QRect r = option.rect.adjusted(4, 2, -4, -2);
    painter->drawText(r, Qt::AlignLeft | Qt::AlignVCenter, text);

    painter->restore();
}

void LineTextDelegate::updateEditorGeometry(QWidget *editor,
                                            const QStyleOptionViewItem &option,
                                            const QModelIndex &) const
{
    QRect r = option.rect;

    // Make editor slightly taller than display rect
    r.adjust(0, -2, 0, 6);

    editor->setGeometry(r);
}

QWidget *LineTextDelegate::createEditor(QWidget *parent,
                                        const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
    QWidget *editor =
        QStyledItemDelegate::createEditor(parent, option, index);

    if (!editor)
        return editor;

    // 🔐 Remove global QSS influence
    editor->setStyleSheet("font: inherit;");

    // Use same font as display
    QFont f = option.font;

    // If you want bigger in edit mode:
    f.setPointSize(f.pointSize() + 2);

    editor->setFont(f);

    return editor;
}

} // namespace Step4
