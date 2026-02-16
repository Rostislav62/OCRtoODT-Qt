#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

// ============================================================
//  OCRtoODT — About Dialog
//  File: aboutdialog.h
//  This dialog shows information about the program.
//  It is loaded from about_dialog.ui
// ============================================================

#include <QDialog>

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog();

private:
    Ui::AboutDialog *ui;
};

#endif // ABOUTDIALOG_H
