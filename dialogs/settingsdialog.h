// ============================================================
//  OCRtoODT — Settings Dialog
//  File: dialogs/settingsdialog.h
//
//  Responsibility:
//      Central dialog hosting all application settings panes.
//
// ============================================================

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

// Forward declarations of panes
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
    void retranslate();
    void onOk();
    void onCancel();
    void onResetToDefaults();
    void onExportConfig();
    void onImportConfig();

private:
    Ui::SettingsDialog *ui = nullptr;

    // Owned panes
    GeneralSettingsPane     *m_general     = nullptr;
    PreprocessSettingsPane  *m_preproc     = nullptr;
    RecognitionSettingsPane *m_recognition = nullptr;
    ODTSettingsPane         *m_odt         = nullptr;
    InterfaceSettingsPane   *m_interface   = nullptr;
    LoggingPane             *m_logging     = nullptr;

    void loadAll();
    bool saveAll();   // ✅ как в Windows
};

#endif // SETTINGSDIALOG_H
