// ============================================================
//  OCRtoODT — About Dialog
//  File: dialogs/aboutdialog.cpp
//
//  Responsibility:
//      - Implement the modal "About" dialog.
//      - Load UI generated from ui/aboutdialog.ui (AUTOUIC).
//      - Wire dialog button box actions to accept/reject.
//
//  Localization rules:
//      - This file must not contain hardcoded user-visible strings.
//      - All user-visible texts are defined in the .ui file and are
//        extracted by lupdate automatically.
// ============================================================

#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include <QString>

// ============================================================
//  Constructor
// ============================================================
AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    ui->setupUi(this);

    ui->lblVersion->setText(
        QString("Version %1").arg(QString::fromUtf8(APP_VERSION))
        );

    // --------------------------------------------------------
    // ButtonBox wiring:
    //  - OK -> accept() closes dialog with Accepted result.
    // --------------------------------------------------------
    connect(ui->buttonBox,
            &QDialogButtonBox::accepted,
            this,
            &QDialog::accept);

    // NOTE:
    // If a Cancel/Close button is added to the UI in the future,
    // connect QDialogButtonBox::rejected to QDialog::reject here.
}

// ============================================================
//  Destructor
// ============================================================
AboutDialog::~AboutDialog()
{
    delete ui;
}

