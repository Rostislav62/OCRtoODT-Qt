// ============================================================
//  OCRtoODT — STEP 4: Edit Lines Controller
//  File: src/4_edit_lines/EditLinesController.h
// ============================================================

// ============================================================
//  OCRtoODT — STEP 4: Edit Lines Controller
//  File: src/4_edit_lines/EditLinesController.h
//
//  Responsibility:
//      Full UI controller for STEP 4 (Text ↔ Preview interaction).
//
//      Owns ALL logic related to:
//          • binding LineTable to Text Tab
//          • inline text editing
//          • Text → Preview synchronization
//          • Preview → Text hit-testing
//          • hover handling
//          • diagnostics and logging
//
//  IMPORTANT FOR SYNCHRONIZATION:
//      • setActivePage() is the ONLY entry point for page changes
//      • Controller must react correctly when page changes externally
//      • Selection changes triggered programmatically MUST NOT
//        recursively trigger synchronization logic
//
//  STRICT RULES:
//      • MainWindow contains NO logic
//      • All connections and event filters live here
// ============================================================

#pragma once

#include <QObject>
#include <QPointer>
#include <QModelIndex>
#include <QRect>
#include <QPoint>

namespace Core {
struct VirtualPage;
}

class QListView;

namespace Input {
class PreviewController;
}

namespace Step4 {

class LineTableModel;

class EditLinesController : public QObject
{
    Q_OBJECT

public:
    explicit EditLinesController(QObject *parent = nullptr);
    ~EditLinesController() override;

    // --------------------------------------------------------
    // UI binding (called once from MainWindow)
    // --------------------------------------------------------
    void attachUi(QListView *listOcrText,
                  Input::PreviewController *previewController);

    // --------------------------------------------------------
    // STEP 3 → STEP 4 data entry point
    // (called whenever ACTIVE PAGE CHANGES)
    // --------------------------------------------------------
    void setActivePage(Core::VirtualPage *page);

    // --------------------------------------------------------
    // UI-only reset (called from MainWindow)
    // --------------------------------------------------------
    void clear();

private slots:
    // --------------------------------------------------------
    // Text → Preview
    // --------------------------------------------------------
    void onTextSelectionChanged(const QModelIndex &current,
                                const QModelIndex &previous);

    void onLineEdited(int pageIndex, int lineOrder, const QString &newText);

    // --------------------------------------------------------
    // Preview → Text
    // --------------------------------------------------------
    void onPreviewHovered(const QPoint &imagePos);
    void onPreviewClicked(const QPoint &imagePos);

private:
    // --------------------------------------------------------
    // Event filter for text hover
    // --------------------------------------------------------
    bool eventFilter(QObject *watched, QEvent *event) override;

    // --------------------------------------------------------
    // Helpers
    // --------------------------------------------------------
    void highlightLine(const QRect &bbox, const char *reason);
    void selectRow(int row, const char *reason);
    int  hitTest(const QPoint &imagePos) const;

    // --------------------------------------------------------
    // STEP 4 debug output helper
    // --------------------------------------------------------
    void dumpLineTableDebug() const;

private:
    QPointer<QListView>                m_list;
    QPointer<Input::PreviewController> m_preview;

    Core::VirtualPage                 *m_page  = nullptr; // non-owning
    Step4::LineTableModel             *m_model = nullptr; // owned

    // --------------------------------------------------------
    // Synchronization guard:
    // prevents recursive selection updates when selection
    // is changed programmatically (Preview → Text)
    // --------------------------------------------------------
    bool m_blockSelection = false;
};

} // namespace Step4
