// ============================================================
//  OCRtoODT — Recognition Settings Pane
//  File: recognitionpane.h
//
//  Responsibility:
//      Defines the QWidget-based settings pane responsible for
//      OCR language management and notification preferences.
//
//      This class:
//          - loads/saves OCR languages from ConfigManager
//          - allows adding custom *.traineddata files
//          - updates active language list
//          - provides notification/sound settings
//
//  The implementation is located in recognitionpane.cpp.
// ============================================================

#ifndef RECOGNITIONPANE_H
#define RECOGNITIONPANE_H

#include <QWidget>
#include <QStringList>

class QSoundEffect;

namespace Ui {
class RecognitionSettingsPane;
}

class RecognitionSettingsPane : public QWidget
{
    Q_OBJECT

public:
    explicit RecognitionSettingsPane(QWidget *parent = nullptr);
    ~RecognitionSettingsPane();

    // Called by SettingsDialog on load/save
    void load();
    void save();


public slots:
    void retranslate();


private slots:
    // User presses "Refresh Languages"
    void onRefreshLanguages();

    // User selects a *.traineddata file to add
    void onAddLanguage();

    // Called when any language is checked/unchecked
    void onLanguageSelectionChanged();

    // Test sound button
    void onTestSound();

    // Volume slider changed
    void onVolumeChanged(int value);

private:
    Ui::RecognitionSettingsPane *ui;
    QSoundEffect *m_soundEffect = nullptr;

    // ----------- Language Management -----------
    void loadLanguages();
    void saveLanguages();
    QStringList getCheckedLanguages() const;
    void updateActiveLanguagesLabel();

    // ----------- Notifications & Sound -----------
    void loadNotificationSettings();
    void saveNotificationSettings();
    void initSoundEffect();
};

#endif // RECOGNITIONPANE_H
