// ============================================================
//  OCRtoODT — STEP 4: Edit Lines Controller
//  File: src/4_edit_lines/EditLinesController.cpp
// ============================================================

// ============================================================
//  OCRtoODT — STEP 4: Edit Lines Controller
//  File: src/4_edit_lines/EditLinesController.cpp
//
//  Responsibility:
//      UI controller for editable OCR lines (Text Tab).
//
//      STEP 4 responsibilities:
//          • bind LineTable → QListView
//          • inline editing (RAM only)
//          • Text → Preview synchronization
//          • Preview → Text hit-test
//          • hover handling
//          • diagnostics (proof that STEP 4 works)
//
//  IMPORTANT FOR SYNCHRONIZATION:
//      • setActivePage() is called whenever the active page changes
//      • Text Tab MUST always reflect the currently selected page
//      • Preview-triggered selections must not recurse
// ============================================================

#include "4_edit_lines/EditLinesController.h"

#include "4_edit_lines/LineTableModel.h"
#include "4_edit_lines/LineHitTest.h"
#include "4_edit_lines/LineTextDelegate.h"

#include "3_LineTextBuilder/LineTable.h"
#include "3_LineTextBuilder/LineTableSerializer.h"

#include "0_input/PreviewController.h"

#include "core/VirtualPage.h"
#include "core/ConfigManager.h"
#include "core/LogRouter.h"

#include <QListView>
#include <QItemSelectionModel>
#include <QEvent>
#include <QMouseEvent>
#include <QAbstractItemView>
#include <QDir>

namespace Step4 {

// ============================================================
// Constructor / Destructor
// ============================================================

EditLinesController::EditLinesController(QObject *parent)
    : QObject(parent)
{
    m_model = new LineTableModel(this);

    LogRouter::instance().info(
        "[STEP4] EditLinesController constructed (model created)");
}

EditLinesController::~EditLinesController() = default;

// ============================================================
// UI binding (called ONCE from MainWindow)
// ============================================================

void EditLinesController::attachUi(QListView *listOcrText,
                                   Input::PreviewController *previewController)
{
    m_list    = listOcrText;
    m_preview = previewController;

    if (!m_list || !m_preview)
    {
        LogRouter::instance().error(
            "[STEP4] attachUi FAILED: null UI pointers");
        return;
    }

    // --------------------------------------------------------
    // Bind model to Text Tab
    // --------------------------------------------------------
    m_list->setModel(m_model);

    // Expanded-height editor for inline editing
    m_list->setItemDelegate(new LineTextDelegate(m_list));

    m_list->setEditTriggers(
        QAbstractItemView::DoubleClicked |
        QAbstractItemView::EditKeyPressed |
        QAbstractItemView::SelectedClicked);

    // Hover tracking
    m_list->setMouseTracking(true);
    m_list->viewport()->installEventFilter(this);

    // --------------------------------------------------------
    // Model → Controller (edit diagnostics)
    // --------------------------------------------------------
    connect(m_model,
            &LineTableModel::lineEdited,
            this,
            &EditLinesController::onLineEdited);

    // --------------------------------------------------------
    // Text → Preview (selection)
    // --------------------------------------------------------
    if (m_list->selectionModel())
    {
        connect(m_list->selectionModel(),
                &QItemSelectionModel::currentChanged,
                this,
                &EditLinesController::onTextSelectionChanged);
    }

    // --------------------------------------------------------
    // Preview → Text
    // --------------------------------------------------------
    connect(m_preview, SIGNAL(imageHoveredAt(QPoint)),
            this, SLOT(onPreviewHovered(QPoint)));

    connect(m_preview, SIGNAL(imageClickedAt(QPoint)),
            this, SLOT(onPreviewClicked(QPoint)));

    LogRouter::instance().info(
        "[STEP4] UI attached: Text Tab READY");
}

// ============================================================
// STEP 3 → STEP 4 entry point (PAGE CHANGE)
// ============================================================

void EditLinesController::setActivePage(Core::VirtualPage *page)
{
    m_page = page;

    // Block selection signals during page switch
    m_blockSelection = true;

    if (!m_page || !m_page->lineTable)
    {
        m_model->setLineTable(nullptr, -1);
        m_blockSelection = false;

        LogRouter::instance().warning(
            "[STEP4] setActivePage: no LineTable → Text Tab CLEARED");
        return;
    }

    // --------------------------------------------------------
    // Populate Text Tab with NEW page data
    // --------------------------------------------------------
    m_model->setLineTable(m_page->lineTable, m_page->globalIndex);

    LogRouter::instance().info(
        QString("[STEP4] Text Tab synchronized: page=%1 rows=%2")
            .arg(m_page->globalIndex)
            .arg(m_page->lineTable->rows.size()));

    // Ensure something is visible
    if (m_model->rowCount() > 0)
        selectRow(0, "page-switch");

    m_blockSelection = false;
}

// ============================================================
// UI-only clear
// ============================================================

void EditLinesController::clear()
{
    m_page = nullptr;
    m_blockSelection = true;

    if (m_model)
        m_model->setLineTable(nullptr, -1);

    if (m_preview)
        m_preview->clearTextHighlight();

    m_blockSelection = false;

    LogRouter::instance().info("[STEP4] EditLinesController cleared");
}

// ============================================================
// Inline editing confirmation + DEBUG OUTPUT
// ============================================================

void EditLinesController::onLineEdited(int pageIndex,
                                       int lineOrder,
                                       const QString &newText)
{
    Q_UNUSED(pageIndex)
    Q_UNUSED(lineOrder)

    LogRouter::instance().info(
        QString("[STEP4] Inline edit OK, newLen=%1")
            .arg(newText.size()));

    dumpLineTableDebug();
}

// ============================================================
// STEP 4 debug / persistence output implementation
//
// Rules:
//  • RAM is always updated first (already done by inline edit)
//  • Disk is written ONLY according to global mode
//  • edit_lines/ is DEBUG HISTORY ONLY (never source of truth)
// ============================================================

void EditLinesController::dumpLineTableDebug() const
{
    if (!m_page || !m_page->lineTable)
        return;

    ConfigManager &cfg = ConfigManager::instance();

    const bool debugMode =
        cfg.get("general.debug_mode", false).toBool();

    const QString execMode =
        cfg.get("general.mode", "ram_only").toString();
    // expected: "ram_only", "disk_only", "ram_then_disk" (or project equivalent)

    const int pageIndex = m_page->globalIndex;

    // --------------------------------------------------------
    // 1) DISK SNAPSHOT for SOURCE-OF-TRUTH when disk is used
    // --------------------------------------------------------
    const bool diskIsSource =
        (execMode == "disk_only" || execMode == "ram_then_disk");

    if (diskIsSource)
    {
        const QString baseDir = "cache/line_text";
        QDir().mkpath(baseDir);

        const QString filePath =
            QString("%1/page_%2.line_table.tsv")
                .arg(baseDir)
                .arg(pageIndex, 4, 10, QLatin1Char('0'));

        if (Tsv::LineTableSerializer::saveToTsv(*m_page->lineTable, filePath))
        {
            LogRouter::instance().info(
                QString("[STEP4][PERSIST] LineTable synced to disk: %1")
                    .arg(filePath));
        }
        else
        {
            LogRouter::instance().error(
                QString("[STEP4][PERSIST] FAILED to sync LineTable: %1")
                    .arg(filePath));
        }
    }

    // --------------------------------------------------------
    // 2) DEBUG HISTORY (edit_lines/) — only if debug enabled
    // --------------------------------------------------------
    if (!debugMode)
        return;

    const QString debugDir = "cache/edit_lines";
    QDir().mkpath(debugDir);

    const QString debugPath =
        QString("%1/page_%2.line_table.tsv")
            .arg(debugDir)
            .arg(pageIndex, 4, 10, QLatin1Char('0'));

    if (Tsv::LineTableSerializer::saveToTsv(*m_page->lineTable, debugPath))
    {
        LogRouter::instance().info(
            QString("[STEP4][DEBUG] Edited LineTable snapshot saved: %1")
                .arg(debugPath));
    }
    else
    {
        LogRouter::instance().warning(
            QString("[STEP4][DEBUG] Failed to save edited LineTable: %1")
                .arg(debugPath));
    }
}


// ============================================================
// Text → Preview
// ============================================================

void EditLinesController::onTextSelectionChanged(
    const QModelIndex &current,
    const QModelIndex &)
{
    if (m_blockSelection || !current.isValid() || !m_page)
        return;

    const auto *row = m_model->rowAt(current.row());
    if (!row)
        return;

    highlightLine(row->bbox, "TextSelection");
}

// ============================================================
// Preview → Text
// ============================================================

void EditLinesController::onPreviewHovered(const QPoint &pos)
{
    if (!m_page)
        return;

    const int row = hitTest(pos);
    if (row >= 0)
        selectRow(row, "PreviewHover");
}

void EditLinesController::onPreviewClicked(const QPoint &pos)
{
    if (!m_page)
        return;

    const int row = hitTest(pos);
    if (row >= 0)
        selectRow(row, "PreviewClick");
}

// ============================================================
// Helpers
// ============================================================

bool EditLinesController::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_list->viewport() &&
        event->type() == QEvent::MouseMove &&
        !m_blockSelection)
    {
        auto *me = static_cast<QMouseEvent*>(event);
        const QModelIndex idx = m_list->indexAt(me->pos());
        if (idx.isValid())
        {
            const auto *row = m_model->rowAt(idx.row());
            if (row)
                highlightLine(row->bbox, "TextHover");
        }
    }
    return QObject::eventFilter(watched, event);
}

int EditLinesController::hitTest(const QPoint &imagePos) const
{
    return (m_page && m_page->lineTable)
    ? LineHitTest::hitTest(m_page->lineTable, imagePos)
    : -1;
}

void EditLinesController::selectRow(int row, const char *reason)
{
    if (!m_list || row < 0 || row >= m_model->rowCount())
        return;

    m_blockSelection = true;

    const QModelIndex idx = m_model->index(row, 0);
    m_list->setCurrentIndex(idx);
    m_list->scrollTo(idx);

    m_blockSelection = false;

    LogRouter::instance().debug(
        QString("[STEP4] Row selected (%1): %2")
            .arg(reason)
            .arg(row));
}

void EditLinesController::highlightLine(const QRect &bbox,
                                        const char *reason)
{
    if (!bbox.isNull() && m_preview)
    {
        m_preview->highlightTextLine(bbox);

        LogRouter::instance().debug(
            QString("[STEP4] Highlight (%1)").arg(reason));
    }
}

} // namespace Step4
