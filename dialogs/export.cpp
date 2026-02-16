// ============================================================
//  OCRtoODT — Export Dialog
//  File: dialogs/export.cpp
//
//  Responsibility:
//      - Provide UI logic for exporting OCR results
//      - Handle format selection and output destination
//      - Dispatch export to TXT / ODT / DOCX controllers
//
//  Localization rules:
//      - ALL user-visible strings must be wrapped in tr()
//      - Format labels are translated here, not in .ui
// ============================================================

#include "dialogs/export.h"
#include "ui_exportdialog.h"

#include <QFileDialog>
#include <QDir>

// STEP 5
#include "5_document/DocumentBuilder.h"
#include "5_document/DocumentDebugWriter.h"
#include "5_export/ExportController.h"

// Core
#include "core/ConfigManager.h"
#include "core/LogRouter.h"

using Core::VirtualPage;

// ============================================================
// Constructor / Destructor
// ============================================================

ExportDialog::ExportDialog(const QVector<VirtualPage>* pages,
                           QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::ExportDialog)
    , m_pages(pages)
{
    ui->setupUi(this);

    // --------------------------------------------------------
    // Initial UI state
    // --------------------------------------------------------
    ui->comboFormat->clear();
    ui->comboFormat->setIconSize(QSize(48, 48));

    ui->comboFormat->addItem(
        QIcon(":/icons/icons/export_txt.svg"),
        tr("TXT (Plain text)"),
        "TXT");

    ui->comboFormat->addItem(
        QIcon(":/icons/icons/export_odt.svg"),
        tr("ODT (LibreOffice)"),
        "ODT");

    ui->comboFormat->addItem(
        QIcon(":/icons/icons/export_docx.svg"),
        tr("DOCX (Microsoft Word)"),
        "DOCX");

    ui->comboFormat->setCurrentIndex(0);

    ui->editFileName->setText(tr("ocr_output"));

    const QString lastDir =
        ConfigManager::instance()
            .get("export.last_dir", QDir::homePath())
            .toString();

    ui->editDir->setText(lastDir);

    // --------------------------------------------------------
    // Connections
    // --------------------------------------------------------
    connect(ui->btnBrowse, &QPushButton::clicked,
            this, &ExportDialog::onBrowseClicked);

    connect(ui->btnOk, &QPushButton::clicked,
            this, &ExportDialog::onOkClicked);

    connect(ui->btnCancel, &QPushButton::clicked,
            this, &ExportDialog::reject);

    connect(ui->comboFormat,
            &QComboBox::currentIndexChanged,
            this,
            [this]()
            {
                const QString format =
                    ui->comboFormat->currentData().toString();

                const QString name =
                    ui->editFileName->text();

                ui->editFileName->setText(
                    normalizedFileName(name, format));
            });
}

ExportDialog::~ExportDialog()
{
    delete ui;
}

// ============================================================
// Browse directory
// ============================================================

void ExportDialog::onBrowseClicked()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Select output directory"),
        ui->editDir->text());

    if (!dir.isEmpty())
        ui->editDir->setText(dir);
}

// ============================================================
// OK → EXPORT
// ============================================================

void ExportDialog::onOkClicked()
{
    if (!m_pages || m_pages->isEmpty())
    {
        LogRouter::instance().warning(
            tr("[ExportDialog] No pages available for export"));
        return;
    }

    const QString outDir = ui->editDir->text().trimmed();
    if (outDir.isEmpty())
    {
        LogRouter::instance().warning(
            tr("[ExportDialog] Output directory is empty"));
        return;
    }

    const QString format =
        ui->comboFormat->currentData().toString();

    const QString fileName =
        normalizedFileName(ui->editFileName->text(), format);

    const QString outPath =
        QDir(outDir).filePath(fileName);

    ConfigManager::instance().set(
        "export.last_dir", outDir);

    accept();

    // --------------------------------------------------------
    // STEP 5 — Build DocumentModel
    // --------------------------------------------------------
    Step5::DocumentBuildOptions opt;
    opt.pageBreak          = true;
    opt.preserveEmptyLines = true;
    opt.maxEmptyLines      = 2;
    opt.preserveLineBreaks = true;
    opt.paragraphPolicy    = Step5::ParagraphPolicy::FromStep3Markers;

    const Step5::DocumentModel doc =
        Step5::DocumentBuilder::build(*m_pages, opt);

    const bool debugEnabled =
        ConfigManager::instance()
            .get("general.debug_mode", false)
            .toBool();

    Step5::DocumentDebugWriter::writeIfEnabled(doc, debugEnabled);

    const bool openAfter =
        ui->checkOpenAfterExport->isChecked();

    if (format == "TXT")
        ExportController::exportTxt(doc, outPath, openAfter);
    else if (format == "ODT")
        ExportController::exportOdt(doc, outPath, openAfter);
    else if (format == "DOCX")
        ExportController::exportDocx(doc, outPath, openAfter);

    LogRouter::instance().info(
        tr("[ExportDialog] Export %1 → %2")
            .arg(format, outPath));
}

// ============================================================
// Helpers
// ============================================================

QString ExportDialog::extensionForFormat(const QString& format) const
{
    if (format == "TXT")  return "txt";
    if (format == "ODT")  return "odt";
    if (format == "DOCX") return "docx";
    return QString();
}

QString ExportDialog::normalizedFileName(const QString& name,
                                         const QString& format) const
{
    QString base = name.trimmed();
    if (base.isEmpty())
        base = tr("ocr_output");

    const QString ext = extensionForFormat(format);
    if (ext.isEmpty())
        return base;

    const int dot = base.lastIndexOf('.');
    if (dot > 0)
        base = base.left(dot);

    return base + "." + ext;
}

