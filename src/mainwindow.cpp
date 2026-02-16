// ============================================================
//  OCRtoODT — Main Window
//  File: src/mainwindow.cpp
//
//  Responsibility:
//      UI shell implementation.
//
//      This file contains ONLY:
//          • UI wiring
//          • signal-slot connections
//          • dialog invocation
//
//      No processing logic is implemented here.
// ============================================================

#include "mainwindow.h"
#include "ui_mainwindow.h"


// ------------------------------------------------------------
// Controllers
// ------------------------------------------------------------

// Text editing controller
#include "4_edit_lines/EditLinesController.h"

// Data processors
#include "core/processors/InputProcessor.h"
#include "core/processors/RecognitionProcessor.h"
#include "core/ConfigManager.h"

// Preview rendering
#include "0_input/PreviewController.h"

// ------------------------------------------------------------
// Dialogs
// ------------------------------------------------------------

#include "dialogs/settingsdialog.h"
#include "dialogs/aboutdialog.h"
#include "dialogs/helpdialog.h"
#include "dialogs/export.h"
#include "dialogs/OcrCompletionDialog.h"

// ------------------------------------------------------------
// Core
// ------------------------------------------------------------

#include "core/VirtualPage.h"
#include "core/LanguageManager.h"


// ============================================================
// Constructor / Destructor
// ============================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // --------------------------------------------------------
    // Progress Manager
    // --------------------------------------------------------
    m_progressManager = new Core::ProgressManager(this);

    connect(m_progressManager,
            &Core::ProgressManager::progressChanged,
            this,
            [this](int value, int max, const QString &text)
            {
                ui->progressTotal->setMaximum(max);
                ui->progressTotal->setValue(value);
                ui->lblStatus->setText(text);
                ui->lblOcrState->setVisible(true);
            });

    connect(m_progressManager,
            &Core::ProgressManager::pipelineFinished,
            this,
            [this](bool ok, const QString &text)
            {
                ui->progressTotal->setValue(
                    ok ? ui->progressTotal->maximum() : 0);

                ui->lblStatus->setText(text.isEmpty()
                                           ? tr("Ready")
                                           : text);
                ui->lblOcrState->setVisible(false);
                ui->actionRun->setEnabled(true);
            });


    connect(ui->actionStop,
            &QAction::triggered,
            this,
            &MainWindow::on_actionStop_triggered);





    // --------------------------------------------------------
    // Subscribe to global language changes
    // --------------------------------------------------------
    connect(LanguageManager::instance(),
            &LanguageManager::languageChanged,
            this,
            &MainWindow::retranslate);

    // Apply initial translation immediately
    retranslate();

    // --------------------------------------------------------
    // Preview controller (visual only)
    // --------------------------------------------------------
    m_previewController =
        new Input::PreviewController(ui->viewPreview, this);

    // --------------------------------------------------------
    // Zoom buttons wiring
    // --------------------------------------------------------
    connect(ui->btnZoomIn,
            &QPushButton::clicked,
            m_previewController,
            &Input::PreviewController::zoomIn);

    connect(ui->btnZoomOut,
            &QPushButton::clicked,
            m_previewController,
            &Input::PreviewController::zoomOut);

    connect(ui->btnZoomFit,
            &QPushButton::clicked,
            m_previewController,
            &Input::PreviewController::zoomFit);

    connect(ui->btnZoom100,
            &QPushButton::clicked,
            m_previewController,
            &Input::PreviewController::zoom100);



    // --------------------------------------------------------
    // Editable text controller
    // --------------------------------------------------------
    m_editLinesController =
        new Step4::EditLinesController(this);

    m_editLinesController->attachUi(
        ui->listOcrText,
        m_previewController);

    // --------------------------------------------------------
    // Input processor (file handling)
    // --------------------------------------------------------
    m_inputProcessor = new InputProcessor(this);

    m_inputProcessor->attachUi(
        ui->listFiles,
        m_previewController);

    // Synchronize page selection with text editor
    connect(m_inputProcessor,
            &InputProcessor::pageActivated,
            this,
            &MainWindow::onPageActivated);

    // --------------------------------------------------------
    // OCR processor
    // --------------------------------------------------------
    m_recognitionProcessor = new RecognitionProcessor(this);

    connect(m_recognitionProcessor,
            &RecognitionProcessor::processingStarted,
            this,
            [this]()
            {
                ui->actionOpen->setEnabled(false);
                ui->actionClear->setEnabled(false);
                ui->actionExport->setEnabled(false);
                ui->actionSettings->setEnabled(false);
                ui->actionRun->setEnabled(false);
                ui->actionStop->setEnabled(true);
            });

    connect(m_recognitionProcessor,
            &RecognitionProcessor::processingFinished,
            this,
            [this]()
            {
                ui->actionOpen->setEnabled(true);
                ui->actionClear->setEnabled(true);
                ui->actionExport->setEnabled(true);
                ui->actionSettings->setEnabled(true);
                ui->actionRun->setEnabled(true);
                ui->actionStop->setEnabled(false);
            });


    m_recognitionProcessor->setProgressManager(m_progressManager);

    connect(m_recognitionProcessor,
            &RecognitionProcessor::ocrCompleted,
            this,
            &MainWindow::onOcrCompleted);

    connect(m_progressManager,
            &Core::ProgressManager::pipelineFinished,
            this,
            [this](bool ok, const QString &msg)
            {
                Q_UNUSED(ok);

                ui->progressTotal->setValue(ui->progressTotal->maximum());
                ui->lblStatus->setText(msg.isEmpty()
                                           ? tr("Ready")
                                           : msg);

                ui->actionRun->setEnabled(true);
            });

}

MainWindow::~MainWindow()
{
    delete ui;
}

// ============================================================
// Language handling
// ============================================================

void MainWindow::retranslate()
{
    // Update all texts generated from the .ui file
    ui->retranslateUi(this);

    // NOTE:
    // If manual texts (menus, dynamic labels, tooltips)
    // are added in the future, they MUST be updated here.
}

// ============================================================
// Menu and toolbar actions
// ============================================================

// Open file selection dialog
void MainWindow::on_actionOpen_triggered()
{
    m_inputProcessor->run(this);
}

// Clear current session and reset UI
void MainWindow::on_actionClear_triggered()
{
    m_inputProcessor->clearSession();

    if (m_editLinesController)
        m_editLinesController->clear();

    if (m_progressManager)
        m_progressManager->reset();

    ui->progressTotal->setValue(0);
    ui->progressTotal->setMaximum(100);
    ui->lblOcrState->setVisible(false);
    ui->lblStatus->setText(tr("Ready"));
}


// Start OCR processing
void MainWindow::on_actionRun_triggered()
{
    ui->actionRun->setEnabled(false);

    const auto jobs = m_inputProcessor->preprocessJobs();

    if (jobs.isEmpty())
    {
        ui->progressTotal->setMaximum(0); // бесконечный режим
        return;
    }

    m_recognitionProcessor->setJobs(jobs);
    m_recognitionProcessor->run();
}

// Export recognized pages
void MainWindow::on_actionExport_triggered()
{
    if (!m_recognitionProcessor)
        return;

    auto &pages = m_recognitionProcessor->pagesMutable();
    if (pages.isEmpty())
        return;

    ExportDialog dlg(&pages, this);
    dlg.exec();
}

// Open settings dialog
void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog dlg(this);
    dlg.exec();
}

// Show application information dialog
void MainWindow::on_actionAbout_triggered()
{
    AboutDialog dlg(this);
    dlg.exec();
}

// Show help / documentation dialog
void MainWindow::on_actionHelp_triggered()
{
    HelpDialog dlg(this);
    dlg.exec();
}

// ============================================================
// Processing lifecycle
// ============================================================

// Activate the first recognized page after OCR completes
void MainWindow::onOcrCompleted(const QVector<Core::VirtualPage> &pages)
{
    Q_UNUSED(pages);

    if (!m_editLinesController || !m_recognitionProcessor)
        return;

    auto &ownedPages = m_recognitionProcessor->pagesMutable();
    if (ownedPages.isEmpty())
        return;

    // --------------------------------------------------------
    // Activate first recognized page
    // --------------------------------------------------------
    m_editLinesController->setActivePage(&ownedPages[0]);

    // --------------------------------------------------------
    // Post-OCR UI notifications (config-driven)
    // --------------------------------------------------------
    ConfigManager &cfg = ConfigManager::instance();


    // Show completion dialog if enabled
    if (cfg.get("ui.notify_on_finish", true).toBool())
    {
        OcrCompletionDialog dlg(this);
        dlg.exec();
    }
}


// ============================================================
// UI synchronization
// ============================================================

// Activate corresponding text page when user selects a file
void MainWindow::onPageActivated(int globalIndex)
{
    if (!m_editLinesController || !m_recognitionProcessor)
        return;

    auto &pages = m_recognitionProcessor->pagesMutable();

    if (globalIndex < 0 || globalIndex >= pages.size())
        return;

    m_editLinesController->setActivePage(&pages[globalIndex]);
}


void MainWindow::on_actionStop_triggered()
{
    if (m_recognitionProcessor)
        m_recognitionProcessor->cancel();

    // UI: allow Run again
    ui->actionRun->setEnabled(true);

    ui->lblOcrState->setVisible(false);
    ui->lblStatus->setText(tr("Ready"));
    ui->progressTotal->setValue(0);

}
