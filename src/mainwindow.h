// ============================================================
//  OCRtoODT — Main Window
//  File: src/mainwindow.h
//
//  Responsibility:
//      Application main window acting as a UI shell.
//
//      This class does NOT implement any processing logic.
//      It only:
//          • wires UI actions to controllers and dialogs
//          • coordinates interaction between UI components
//          • reacts to high-level application events
//
//  Strict rules:
//      • NO OCR logic
//      • NO TSV parsing
//      • NO pipeline algorithms
//      • NO text recognition logic
// ============================================================

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

#include "core/ProgressManager.h"


// Forward declarations
namespace Ui { class MainWindow; }

class InputProcessor;
class RecognitionProcessor;

namespace Core { struct VirtualPage; }
namespace Input { class PreviewController; }
namespace Step4 { class EditLinesController; }

class MainWindow : public QMainWindow
{
    Q_OBJECT

protected:
    void closeEvent(QCloseEvent *event) override;

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

public slots:
    // --------------------------------------------------------
    // Language handling
    // --------------------------------------------------------

    // Re-translate all user-visible texts in the main window
    void retranslate();

    // --------------------------------------------------------
    // Menu and toolbar actions
    // --------------------------------------------------------

    // Open file selection dialog and add files to the session
    void on_actionOpen_triggered();

    // Clear the current session and reset UI state
    void on_actionClear_triggered();

    // Start OCR processing for the currently loaded files
    void on_actionRun_triggered();

    // Export recognized pages (placeholder entry point)
    void on_actionExport_triggered();

    // Open application settings dialog
    void on_actionSettings_triggered();

    // Show "About" dialog
    void on_actionAbout_triggered();

    // Show help / documentation dialog
    void on_actionHelp_triggered();

    // --------------------------------------------------------
    // Processing lifecycle notifications
    // --------------------------------------------------------

    // Called when OCR processing finishes and pages are available
    void onOcrCompleted(const QVector<Core::VirtualPage> &pages);

    // --------------------------------------------------------
    // UI synchronization
    // --------------------------------------------------------

    // Activate a page in the text editor when user selects it in the file list
    void onPageActivated(int globalIndex);

    void on_actionStop_triggered();


private:

    Ui::MainWindow *ui = nullptr;

    enum class AppState
    {
        IdleEmpty,
        Loaded,
        Running,
        Completed
    };

    AppState computeState() const;
    void updateUiState();
    void attemptShutdown();

    bool m_shutdownRequested = false;

    // Visual-only controller responsible for preview rendering
    Input::PreviewController *m_previewController = nullptr;

    // Handles file input and preprocessing
    InputProcessor *m_inputProcessor = nullptr;

    // Controls OCR execution and owns recognized pages
    RecognitionProcessor *m_recognitionProcessor = nullptr;

    // Manages editable OCR text view
    Step4::EditLinesController *m_editLinesController = nullptr;

    // Progress Manager
    Core::ProgressManager *m_progressManager = nullptr;
};

#endif // MAINWINDOW_H
