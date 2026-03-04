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
#include <QCloseEvent>
#include <QTimer>


// ------------------------------------------------------------
// Controllers
// ------------------------------------------------------------

// Text editing controller
#include "4_edit_lines/EditLinesController.h"

// Data processors
#include "core/processors/InputProcessor.h"
#include "core/processors/RecognitionProcessor.h"
#include "core/ConfigManager.h"
#include "core/LogRouter.h"
#include "core/ocr/OcrLanguageManager.h"

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

    // ============================================================
    //  OCR Result view tuning — allow A4-like text wrapping
    // ============================================================

    ui->listOcrText->setWordWrap(true);
    ui->listOcrText->setUniformItemSizes(false);


    // ============================================================
    //  Layout tuning — A4 oriented proportions
    // ============================================================

    // Initial splitter proportions (Left / Center / Right)
    {
        QList<int> sizes;

        const int total = width() > 0 ? width() : 1500;

        // -10% thumbnails
        int left  = total * 0.18;   // было визуально ~20%
        // -15% preview
        int center = total * 0.40;  // было ~40-45%
        // всё остальное OCR
        int right = total - left - center;

        sizes << left << center << right;
        ui->splitterMain->setSizes(sizes);

        // OCR panel должен расширяться активнее
        ui->splitterMain->setStretchFactor(0, 0); // left
        ui->splitterMain->setStretchFactor(1, 1); // preview
        ui->splitterMain->setStretchFactor(2, 2); // OCR result
    }

    // Minimum widths to avoid collapsing into unusable columns
    ui->panelLeft->setMinimumWidth(180);
    ui->panelCenter->setMinimumWidth(350);
    ui->panelRight->setMinimumWidth(450);

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

    connect(&OcrLanguageManager::instance(),
            &OcrLanguageManager::languagesChanged,
            this,
            &MainWindow::onOcrLanguagesChanged);

    // --------------------------------------------------------
    // Initial UI state
    // --------------------------------------------------------
    updateUiState();

    connect(m_recognitionProcessor,
            &RecognitionProcessor::processingStarted,
            this,
            [this]()
            {
                updateUiState();

            });

    connect(m_recognitionProcessor,
            &RecognitionProcessor::processingFinished,
            this,
            [this]()
            {
                updateUiState();

                // 🔐 STEP 4.2 — if shutdown was requested,
                // allow application to close now.
                attemptShutdown();
            });


    m_recognitionProcessor->setProgressManager(m_progressManager);

    connect(m_recognitionProcessor,
            &RecognitionProcessor::ocrCompleted,
            this,
            &MainWindow::onOcrCompleted);

    connect(m_progressManager,
            &Core::ProgressManager::pipelineFinished,
            this,
            [this](bool ok, const QString &text)
            {
                ui->progressTotal->setValue(ok ? ui->progressTotal->maximum() : 0);

                ui->lblStatus->setText(text.isEmpty() ? tr("Ready") : text);
                ui->lblOcrState->setVisible(false);

                updateUiState();
            });

    connect(m_inputProcessor,
            &InputProcessor::inputStateChanged,
            this,
            [this]()
            {
                updateUiState();
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
    if (m_inputProcessor)
        m_inputProcessor->clearSession();

    if (m_recognitionProcessor)
        m_recognitionProcessor->clearSession();

    if (m_editLinesController)
        m_editLinesController->clear();

    if (m_previewController)
        m_previewController->reset();

    if (m_progressManager)
        m_progressManager->reset();

    ui->progressTotal->setValue(0);
    ui->progressTotal->setMaximum(100);
    ui->lblOcrState->setVisible(false);
    ui->lblStatus->setText(tr("Ready"));

    updateUiState();

    m_pendingRunAfterLangDownload = false;
    m_autoRerunArmed = false;
}


// Start OCR processing
void MainWindow::on_actionRun_triggered()
{
    if (!m_inputProcessor || !m_recognitionProcessor)
        return;

    // Если уже идет OCR — игнор (защита от двойного RUN)
    if (m_recognitionProcessor->isProcessing())
        return;

    const auto jobs = m_inputProcessor->preprocessJobs();

    // Нечего распознавать → просто сообщаем и выходим
    if (jobs.isEmpty())
    {
        ui->lblStatus->setText(tr("No input loaded"));
        updateUiState();
        return;
    }

    auto& lm = OcrLanguageManager::instance();

    if (!lm.ensureActiveLanguagesReady())
    {
        m_pendingRunAfterLangDownload = true;
        ui->lblStatus->setText(tr("Downloading OCR language data…"));
        updateUiState();
        return;
    }

    // Стартуем OCR
    m_recognitionProcessor->setJobs(jobs);
    m_recognitionProcessor->run();

    updateUiState();
}

// Export recognized pages
void MainWindow::on_actionExport_triggered()
{
    if (!m_recognitionProcessor)
        return;

    auto &pages = m_recognitionProcessor->pagesMutable();
    if (pages.isEmpty())
    {
        ui->lblStatus->setText(tr("Nothing to export"));
        updateUiState();
        return;
    }

    ExportDialog dlg(&pages, this);
    dlg.exec();

    updateUiState();
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

void MainWindow::on_actionStop_triggered()
{
    if (!m_recognitionProcessor)
        return;

    if (!m_recognitionProcessor->isProcessing())
    {
        updateUiState();
        return;
    }

    m_recognitionProcessor->cancel();

    ui->lblStatus->setText(tr("Stopping OCR..."));
    updateUiState();
}

// ============================================================
// Processing lifecycle
// ============================================================

// Activate the first recognized page after OCR completes
void MainWindow::onOcrCompleted(const QVector<Core::VirtualPage> &pages)
{
    updateUiState();

    Q_UNUSED(pages);

    if (!m_editLinesController || !m_recognitionProcessor)
        return;

    LogRouter::instance().info(
        QString("[MainWindow] onOcrCompleted signal received: pages param=%1").arg(pages.size()));

    auto &ownedPages = m_recognitionProcessor->pagesMutable();
    LogRouter::instance().info(
        QString("[MainWindow] ownedPages after recognition=%1").arg(ownedPages.size()));

    if (!ownedPages.isEmpty())
    {
        LogRouter::instance().info(
            QString("[MainWindow] owned page0: idx=%1 lineTable=%2")
                .arg(ownedPages[0].globalIndex)
                .arg(ownedPages[0].lineTable ? "YES" : "NO"));
    }

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

// ============================================================
// UI State Machine
// ============================================================

MainWindow::AppState MainWindow::computeState() const
{
    const bool isRunning =
        m_recognitionProcessor &&
        m_recognitionProcessor->isProcessing();

    const bool hasInput =
        (ui->listFiles && ui->listFiles->model() && ui->listFiles->model()->rowCount() > 0);

    const bool hasResults =
        m_recognitionProcessor &&
        !m_recognitionProcessor->pagesMutable().isEmpty();

    if (isRunning)
        return AppState::Running;

    if (!hasInput)
        return AppState::IdleEmpty;

    if (hasInput && !hasResults)
        return AppState::Loaded;

    return AppState::Completed;
}

void MainWindow::updateUiState()
{
    const AppState state = computeState();

    switch (state)
    {
    case AppState::IdleEmpty:
        ui->actionRun->setEnabled(false);
        ui->actionClear->setEnabled(false);
        ui->actionExport->setEnabled(false);
        ui->actionStop->setEnabled(false);
        break;

    case AppState::Loaded:
        ui->actionRun->setEnabled(true);
        ui->actionClear->setEnabled(true);
        ui->actionExport->setEnabled(false);
        ui->actionStop->setEnabled(false);
        break;

    case AppState::Running:
        ui->actionRun->setEnabled(false);
        ui->actionClear->setEnabled(false);
        ui->actionExport->setEnabled(false);
        ui->actionStop->setEnabled(true);
        break;

    case AppState::Completed:
        ui->actionRun->setEnabled(true);
        ui->actionClear->setEnabled(true);
        ui->actionExport->setEnabled(true);
        ui->actionStop->setEnabled(false);
        break;
    }
}


// ============================================================
// Safe shutdown (STEP 4.2)
// ============================================================

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Если уже в процессе shutdown — пропускаем
    if (m_shutdownRequested)
    {
        event->accept();
        return;
    }

    // Если OCR не запущен — закрываем сразу
    if (!m_recognitionProcessor)
    {
        event->accept();
        return;
    }

    // Проверяем: идет ли обработка
    // (processingFinished эмитится по завершению)
    if (!m_recognitionProcessor->isProcessing())
    {
        // OCR не активен
        event->accept();
        return;
    }

    // OCR активен → инициируем безопасный shutdown
    m_shutdownRequested = true;

    ui->lblStatus->setText(tr("Stopping OCR..."));
    ui->actionStop->setEnabled(false);

    LogRouter::instance().info(
        QString("[STATE] UI event=STOP_CLICK run=%1")
            .arg(m_recognitionProcessor ?
                     m_recognitionProcessor->currentRunId() : 0));

    m_recognitionProcessor->cancel();

    event->ignore();   // 🚫 Запрещаем закрытие до завершения
}

// ------------------------------------------------------------

void MainWindow::attemptShutdown()
{
    if (!m_shutdownRequested)
        return;

    // Разрешаем закрытие после полного завершения pipeline
    m_shutdownRequested = false;
    close();
}

void MainWindow::onOcrLanguagesChanged()
{
    // Авто-повтор нужен только если мы ранее заблокировали RUN из-за скачивания
    if (!m_pendingRunAfterLangDownload)
        return;

    // Если OCR сейчас работает — ничего не делаем, подождем следующего изменения
    if (m_recognitionProcessor && m_recognitionProcessor->isProcessing())
        return;

    // Проверяем: все ли языки уже готовы.
    // ВАЖНО: ensureActiveLanguagesReady() может стартовать недостающие докачки.
    auto& lm = OcrLanguageManager::instance();
    if (!lm.ensureActiveLanguagesReady())
    {
        // Еще не готово — остаемся в режиме ожидания
        ui->lblStatus->setText(tr("Downloading OCR language data…"));
        updateUiState();
        return;
    }

    // Готово → запускаем RUN один раз через event loop (без реэнтранси)
    if (m_autoRerunArmed)
        return;

    m_autoRerunArmed = true;

    QTimer::singleShot(0, this, [this]()
                       {
                           m_autoRerunArmed = false;

                           // Могли успеть очистить/закрыть — перепроверим
                           if (!m_pendingRunAfterLangDownload)
                               return;

                           if (!m_inputProcessor || !m_recognitionProcessor)
                               return;

                           if (m_recognitionProcessor->isProcessing())
                               return;

                           // Снимаем флаг ДО запуска, чтобы если снова начнется download,
                           // мы могли снова корректно поставить флаг.
                           m_pendingRunAfterLangDownload = false;

                           ui->lblStatus->setText(tr("Starting OCR…"));
                           updateUiState();

                           on_actionRun_triggered();
                       });
}
