// ============================================================
//  OCRtoODT — Settings: Preprocess Pane
//  File: settings/preprocesspane.h
//
//  Responsibility:
//      - Display preprocessing profile selection
//      - Show short description
//      - Show/hide advanced parameters depending on expert mode
//      - Provide "Reset current profile to defaults" action
//        via an extra combo item (no extra buttons)
//
//      STEP 3 (Protection / Recoverability):
//          - In Expert mode, destructive reset requires confirmation.
//          - Default profiles are stored in a single table-driven block
//            (one source of truth for baseline values).
//
//  IMPORTANT DESIGN RULES:
//      - This pane NEVER applies image processing itself
//      - This pane NEVER reloads pipeline
//      - It only edits config values
// ============================================================

#ifndef PREPROCESSPANE_H
#define PREPROCESSPANE_H

#include <QWidget>
#include <QString>

namespace Ui {
class PreprocessSettingsPane;
}

class PreprocessSettingsPane : public QWidget
{
    Q_OBJECT

public:
    explicit PreprocessSettingsPane(QWidget *parent = nullptr);
    ~PreprocessSettingsPane();

    void load();
    void save();

public slots:
    // Called when expert mode is toggled in General pane
    void setExpertMode(bool enabled);

private slots:
    void onProfileChanged(int index);
    void retranslate();

private:
    Ui::PreprocessSettingsPane *ui;

    // Last "real" profile selection (0..3), used when user clicks reset item.
    int  m_lastProfileIndex = 1; // default: scanner
    bool m_expertMode       = false;

private:
    void updateDescription(int index);

    // Apply baseline defaults into config for a given profile key.
    void resetProfileToDefaults(const QString &profileKey);

    // Helper: get "real profile index" (0..3) even if reset is clicked.
    int normalizedProfileIndex(int index) const;

    // Expert-mode destructive action confirm.
    bool confirmResetInExpertMode(const QString &profileKey);
};

#endif // PREPROCESSPANE_H
