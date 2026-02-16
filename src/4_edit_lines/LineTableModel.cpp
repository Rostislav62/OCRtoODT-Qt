// ============================================================
//  OCRtoODT — STEP 4: LineTable Model (Editable OCR Lines)
//  File: src/4_edit_lines/LineTableModel.cpp
// ============================================================

#include "4_edit_lines/LineTableModel.h"

#include "3_LineTextBuilder/LineTable.h"   // Tsv::LineTable, Tsv::LineRow
#include "core/LogRouter.h"

namespace Step4 {

LineTableModel::LineTableModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void LineTableModel::setLineTable(Tsv::LineTable *table, int pageIndex)
{
    beginResetModel();

    m_table = table;
    m_pageIndex = pageIndex;

    endResetModel();

    // Diagnostic: proves Text Tab must show N rows after binding.
    LogRouter::instance().info(
        QString("[LineTableModel] setLineTable: page=%1 table=%2 rows=%3")
            .arg(m_pageIndex)
            .arg(m_table ? "OK" : "NULL")
            .arg(m_table ? m_table->rows.size() : 0));
}

const Tsv::LineRow* LineTableModel::rowAt(int row) const
{
    if (!m_table)
        return nullptr;

    if (row < 0 || row >= m_table->rows.size())
        return nullptr;

    return &m_table->rows[row];
}

int LineTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    if (!m_table)
        return 0;

    return m_table->rows.size();
}

QVariant LineTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_table)
        return QVariant();

    const int r = index.row();
    if (r < 0 || r >= m_table->rows.size())
        return QVariant();

    const auto &row = m_table->rows[r];

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return row.text;

    case RoleText:        return row.text;
    case RoleBbox:        return row.bbox;
    case RolePageIndex:   return row.pageIndex;
    case RoleLineOrder:   return row.lineOrder;
    case RoleBlockNum:    return row.blockNum;
    case RoleParNum:      return row.parNum;
    case RoleLineNum:     return row.lineNum;
    case RoleAvgConf:     return row.avgConf;
    case RoleWordCount:   return row.wordCount;

    case RoleIsSyntheticEmpty:
        // STEP 3 convention: synthetic empty rows have ids < 0
        return (row.blockNum < 0 && row.parNum < 0 && row.lineNum < 0);

    default:
        return QVariant();
    }
}

bool LineTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!m_table || !index.isValid())
        return false;

    if (role != Qt::EditRole && role != RoleText)
        return false;

    const int r = index.row();
    if (r < 0 || r >= m_table->rows.size())
        return false;

    auto &row = m_table->rows[r];

    const QString newText = value.toString();
    if (row.text == newText)
    {
        LogRouter::instance().debug(
            QString("[LineTableModel] setData: unchanged page=%1 row=%2 lineOrder=%3")
                .arg(m_pageIndex)
                .arg(r)
                .arg(row.lineOrder));
        return true;
    }

    row.text = newText;

    // Diagnostic: proves inline edit changed RAM and view is notified.
    LogRouter::instance().info(
        QString("[LineTableModel] setData: edited page=%1 row=%2 lineOrder=%3 newLen=%4")
            .arg(m_pageIndex)
            .arg(r)
            .arg(row.lineOrder)
            .arg(newText.size()));

    emit dataChanged(index, index, {Qt::DisplayRole, Qt::EditRole, RoleText});
    emit lineEdited(m_pageIndex, row.lineOrder, newText);

    return true;
}

Qt::ItemFlags LineTableModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    // Editable for ALL rows, including synthetic empty rows.
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QHash<int, QByteArray> LineTableModel::roleNames() const
{
    QHash<int, QByteArray> r;
    r[RoleText]            = "text";
    r[RoleBbox]            = "bbox";
    r[RolePageIndex]       = "pageIndex";
    r[RoleLineOrder]       = "lineOrder";
    r[RoleBlockNum]        = "blockNum";
    r[RoleParNum]          = "parNum";
    r[RoleLineNum]         = "lineNum";
    r[RoleAvgConf]         = "avgConf";
    r[RoleWordCount]       = "wordCount";
    r[RoleIsSyntheticEmpty]= "isSyntheticEmpty";
    return r;
}

} // namespace Step4
