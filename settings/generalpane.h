// ============================================================
//  OCRtoODT — Settings: General Pane
//  File: settings/generalpane.h
//
//  Responsibility:
//      Provides UI for the "General" tab inside SettingsDialog.
//
//      Execution Strategy:
//          - Automatic
//          - Sequential
//          - Parallel
//
//      Control Level:
//          - Standard
//          - Professional
//
//      Public API:
//          load()  → fill UI from config.yaml
//          save()  → write UI values back to ConfigManager
//
// ============================================================

#ifndef GENERALPANE_H
#define GENERALPANE_H

#include <QWidget>

namespace Ui {
class GeneralSettingsPane;
}

class GeneralSettingsPane : public QWidget
{
    Q_OBJECT

public:
    explicit GeneralSettingsPane(QWidget *parent = nullptr);
    ~GeneralSettingsPane();

    void load();
    void save();

public slots:
    void retranslate();

private:
    Ui::GeneralSettingsPane *ui;
};

#endif // GENERALPANE_H
