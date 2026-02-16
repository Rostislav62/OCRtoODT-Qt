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
    Q_UNUSED(index);

    QFontMetrics fm(option.font);
    return QSize(option.rect.width(), fm.height() + 6);
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

} // namespace Step4
