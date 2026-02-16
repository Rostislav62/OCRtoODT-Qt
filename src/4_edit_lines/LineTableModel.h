// ============================================================
//  OCRtoODT — STEP 4: LineTable Model (Editable OCR Lines)
//  File: src/4_edit_lines/LineTableModel.h
//
//  Responsibility:
//      Qt editable model over Tsv::LineTable for the Text Tab.
//      - One row == one LineRow
//      - Inline edit modifies LineRow::text in RAM
//      - Emits lineEdited(...) for diagnostics and future persistence
//
//  Notes:
//      - Model does NOT own LineTable memory (owned by VirtualPage).
//      - Caller must keep LineTable alive while it is bound.
// ============================================================

#pragma once

#include <QAbstractListModel>
#include <QRect>
#include <QHash>

namespace Tsv {
struct LineTable;
struct LineRow;
}

namespace Step4 {

class LineTableModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles
    {
        RoleText = Qt::UserRole + 1,
        RoleBbox,
        RolePageIndex,
        RoleLineOrder,
        RoleBlockNum,
        RoleParNum,
        RoleLineNum,
        RoleAvgConf,
        RoleWordCount,
        RoleIsSyntheticEmpty
    };

public:
    explicit LineTableModel(QObject *parent = nullptr);

    // LineTable is NOT owned. Can be null (clears view).
    void setLineTable(Tsv::LineTable *table, int pageIndex);
    Tsv::LineTable* lineTable() const { return m_table; }

    int pageIndex() const { return m_pageIndex; }

    // Convenience accessor for controller (safe)
    const Tsv::LineRow* rowAt(int row) const;

    // QAbstractItemModel
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    // Diagnostics hook: proves inline edit changes RAM model state.
    void lineEdited(int pageIndex, int lineOrder, const QString &newText);

private:
    Tsv::LineTable *m_table = nullptr; // not owned
    int m_pageIndex = -1;
};

} // namespace Step4
