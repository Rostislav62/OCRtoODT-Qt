// ============================================================
//  OCRtoODT — Settings Dialog
//  File: dialogs/settingsdialog.h
//
//  Responsibility:
//      Central dialog that hosts all application settings panes.
//
//      This dialog is responsible for:
//          • creating and owning settings panes
//          • loading settings from ConfigManager into panes
//          • saving pane state back into ConfigManager
//          • applying runtime-reloadable settings immediately
//          • updating UI language live
//
//      This class does NOT:
//          • contain business logic
//          • perform validation unrelated to UI
// ============================================================

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

// Forward declarations of settings panes
class GeneralSettingsPane;
class PreprocessSettingsPane;
class RecognitionSettingsPane;
class ODTSettingsPane;
class InterfaceSettingsPane;
class LoggingPane;

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;

public slots:
    // Re-translate all user-visible texts in this dialog
    void retranslate();

private slots:
    // Apply settings and close dialog
    void onOk();

    // Discard changes and close dialog
    void onCancel();

private:
    Ui::SettingsDialog *ui = nullptr;

    // Modular settings panes (owned by dialog)
    GeneralSettingsPane     *m_general     = nullptr;
    PreprocessSettingsPane  *m_preproc     = nullptr;
    RecognitionSettingsPane *m_recognition = nullptr;
    ODTSettingsPane         *m_odt         = nullptr;
    InterfaceSettingsPane   *m_interface   = nullptr;
    LoggingPane             *m_logging     = nullptr;

private:
    // Load settings from ConfigManager into all panes
    void loadAll();

    // Save all panes back into ConfigManager
    void saveAll();
};

#endif // SETTINGSDIALOG_H
