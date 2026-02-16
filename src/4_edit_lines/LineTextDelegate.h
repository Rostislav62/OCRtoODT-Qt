// ============================================================
//  OCRtoODT — STEP 4: LineTextDelegate (optional visual policy)
//  File: src/4_edit_lines/LineTextDelegate.h
//
//  Responsibility:
//      Visual + editing policy for listOcrText rows:
//          - row height (e.g., empty lines taller)
//          - optional coloring by confidence / state
//
//  NOTE:
//      Delegate does NOT implement mapping logic (controller does).
// ============================================================

#pragma once

#include <QStyledItemDelegate>

namespace Step4 {

class LineTextDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit LineTextDelegate(QObject *parent = nullptr);

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};


} // namespace Step4
