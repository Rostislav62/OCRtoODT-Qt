// ============================================================
//  OCRtoODT — Input Processor
//  File: src/core/processors/InputProcessor.h
//
//  Responsibility:
//
//      STEP 0_input:
//          • Open file dialog
//          • Expand images / PDF into pages
//          • Copy pages to cache/input/
//          • Rename pages as 0001.ext
//          • Build file list model
//          • Generate thumbnails
//          • Show ORIGINAL preview
//
//      STEP 1_preprocess:
//          • Wait until STEP 0 finished
//          • Run PreprocessPipeline (parallel)
//          • Optionally save enhanced images
//          • Switch thumbnails + preview to enhanced images
//
//      IMPORTANT:
//          • Provides STEP 0 output pages via pages() const.
//          • pages() is a contract used by MainWindow/STEP 2.
//
// ============================================================

#ifndef INPUTPROCESSOR_H
#define INPUTPROCESSOR_H

#include <QObject>
#include <QVector>
#include <QHash>

// ------------------------------------------------------------
// Project types used by value (must be fully included)
// ------------------------------------------------------------
#include "core/VirtualPage.h"
#include "1_preprocess/PageJob.h"

// ------------------------------------------------------------
// Qt forward
// ------------------------------------------------------------
class QWidget;
class QListView;
class QStandardItemModel;
class QTimer;
class QImage;

namespace Input {
class InputController;
class PreviewController;
}

namespace Ocr::Preprocess {
class PreprocessPipeline;
}

// ============================================================
// InputProcessor
// ============================================================
class InputProcessor : public QObject
{
    Q_OBJECT


signals:
    // STEP 0 → UI sync
    void pageActivated(int globalIndex);

    void inputStateChanged();

public:
    explicit InputProcessor(QObject *parent = nullptr);

    // ------------------------------------------------------------
    // Access prepared input pages (STEP 0 output)
    //
    // Contract:
    //  - Always returns current snapshot of STEP 0 pages
    //  - Empty if STEP 0 not completed yet
    // ------------------------------------------------------------
    const QVector<Core::VirtualPage>& pages() const;

    // --------------------------------------------------------
    // Attach UI widgets (called once from MainWindow)
    // --------------------------------------------------------
    void attachUi(QListView *listFiles,
                  Input::PreviewController *previewController);

    // --------------------------------------------------------
    // Scenario entry (STEP 0 → STEP 1)
    // --------------------------------------------------------
    void run(QWidget *parentWidget);

    // --------------------------------------------------------
    // Full session reset (used by "Clear" and before new run)
    // --------------------------------------------------------
    void clearSession();

    // ------------------------------------------------------------
    // Access STEP 1 preprocess results
    // ------------------------------------------------------------
    QVector<Ocr::Preprocess::PageJob> preprocessJobs() const;

public slots:
    // UI: apply "ui.thumbnail_size" to the real file list (100..200 clamp)
    void applyThumbnailSizeFromConfig();

private:
    // --------------------------------------------------------
    // Controllers
    // --------------------------------------------------------
    Input::InputController          *m_inputController     = nullptr;
    Ocr::Preprocess::PreprocessPipeline *m_preprocessPipeline = nullptr;

    // --------------------------------------------------------
    // UI (not owned)
    // --------------------------------------------------------
    QListView                *m_listFiles         = nullptr;
    Input::PreviewController *m_previewController = nullptr;

    // --------------------------------------------------------
    // STEP 0 state
    // --------------------------------------------------------
    QStandardItemModel *m_model = nullptr;
    int m_expectedPages = 0;

    // STEP 0 output (stable storage for pages() const)
    QVector<Core::VirtualPage> m_pages;

    // --------------------------------------------------------
    // STEP 1 state
    // --------------------------------------------------------
    QHash<int, Ocr::Preprocess::PageJob> m_jobsByIndex;
    bool m_showFinalPreview = true;
    bool m_step1Running = false;

    QTimer *m_step1PollTimer = nullptr;

    // --------------------------------------------------------
    // Internal wiring
    // --------------------------------------------------------
    void wire();

    // --------------------------------------------------------
    // STEP 1 helpers
    // --------------------------------------------------------
    void startStep1Polling();
    bool isStep0Complete() const;

    // Builds and stores STEP 0 pages snapshot into m_pages
    // and returns it by const reference.
    const QVector<Core::VirtualPage>& rebuildPagesFromCacheAndModel();

    void runStep1Preprocess(const QVector<Core::VirtualPage> &pages);

    void applyEnhancedThumbnails();
    void showPreviewAccordingToConfig(const Core::VirtualPage &vp,
                                      const QImage &originalImg);
};

#endif // INPUTPROCESSOR_H
