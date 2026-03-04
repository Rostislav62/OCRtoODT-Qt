// ============================================================
//  OCRtoODT — Recognition Settings Pane
//  File: settings/recognitionpane.h
//
//  GoldenDict-style dual list architecture + Profiles UI
//
//  DESIGN RULES:
//      - No OCR logic here
//      - UI talks to OcrLanguageManager ONLY (not config)
// ============================================================

#pragma once

#include <QWidget>
#include <QMap>
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
    ~RecognitionSettingsPane() override;

    // --------------------------------------------------------
    // Called by SettingsDialog on open / accept
    // --------------------------------------------------------
    void load();

    // Returns false if validation fails (e.g. empty active profile)
    bool save();

private slots:
    // --------------------------------------------------------
    // Profiles
    // --------------------------------------------------------
    void onProfileChanged(int index);
    void onAddProfile();
    void onDeleteProfile();
    void onRenameProfile();

    // --------------------------------------------------------
    // Languages (GoldenDict-style)
    // --------------------------------------------------------
    void onAddToActive();
    void onRemoveFromActive();
    void onRefreshLanguages();
    void onInstallLanguage();

    // --------------------------------------------------------
    // Notifications & sound
    // --------------------------------------------------------
    void onTestSound();
    void onVolumeChanged(int value);

    void retranslate();

private:
    // --------------------------------------------------------
    // UI load helpers
    // --------------------------------------------------------
    void loadProfiles();
    void loadLanguages();

    void loadNotificationSettings();
    void saveNotificationSettings();
    void initSoundEffect();

    // --------------------------------------------------------
    // UI item helpers (store code in UserRole)
    // --------------------------------------------------------
    void addLangItemToList(class QListWidget* list, const QString& code);
    static QString codeFromItem(const class QListWidgetItem* item);

    Ui::RecognitionSettingsPane *ui = nullptr;
    QSoundEffect *m_soundEffect = nullptr;

    // --------------------------------------------------------
    // Pending (staged) profile model
    // Changes are committed ONLY on SettingsDialog OK.
    // --------------------------------------------------------
    QString m_pendingActiveProfile;
    QMap<QString, QStringList> m_pendingProfileLangs;

    QStringList pendingLanguages(const QString& profile) const;
    void setPendingLanguages(const QString& profile, const QStringList& langs);

    // Convenience barrier: refresh lists from pending model
    void reloadListsFromPending();
};
