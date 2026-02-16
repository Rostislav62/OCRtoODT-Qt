// ============================================================
//  OCRtoODT — Interface Settings Pane
//  File: settings/interfacepane.h
//
//  Responsibility:
//      Provides the "Interface" tab inside SettingsDialog.
//
//      High-level responsibilities:
//          - Load interface-related settings from ConfigManager
//          - Persist user changes back into ConfigManager
//          - Provide a live, isolated preview of interface appearance
//          - Forward global language changes to local retranslation
//
//      Design rules:
//          - This pane does NOT install or own QTranslator
//          - Global language switching is handled by LanguageManager
//          - This pane reacts to language changes via retranslate()
//          - Preview changes MUST NOT affect the real MainWindow
// ============================================================

#ifndef INTERFACEPANE_H
#define INTERFACEPANE_H

#include <QWidget>
#include <QPixmap>
#include <QStringList>

class QAbstractButton;
class QFont;

namespace Ui {
class InterfaceSettingsPane;
}

class InterfaceSettingsPane : public QWidget
{
    Q_OBJECT

public:
    explicit InterfaceSettingsPane(QWidget *parent = nullptr);
    ~InterfaceSettingsPane() override;

    // Load values from ConfigManager into UI widgets
    void load();

    // Save UI values back into ConfigManager
    void save();

protected:
    // Keeps document preview image scaled correctly when pane is resized
    void resizeEvent(QResizeEvent *event) override;

signals:
    // Emitted after settings are saved and should be applied live
    void uiSettingsChanged();

private slots:
    // ------------------------------------------------------------
    // Appearance
    // ------------------------------------------------------------
    void onThemeModeChanged(int);
    void onLoadCustomQss();

    // ------------------------------------------------------------
    // Fonts
    // ------------------------------------------------------------
    void onAppFontChanged(const QFont &);
    void onAppFontSizeChanged(int);

    // ------------------------------------------------------------
    // Toolbar
    // ------------------------------------------------------------
    void onToolbarStyleChanged();
    void onToolbarIconSizeChanged(int);

    // ------------------------------------------------------------
    // Thumbnails
    // ------------------------------------------------------------
    void onThumbSizeChanged(int);

    // ------------------------------------------------------------
    // Preview pages
    // ------------------------------------------------------------
    void onPreviewPageSelected(int row);

    // ------------------------------------------------------------
    // Language
    // ------------------------------------------------------------
    void onLanguageComboChanged(int);
    void onGlobalLanguageChanged(const QString &langCode);

private:
    Ui::InterfaceSettingsPane *ui = nullptr;

    // ------------------------------------------------------------
    // Load / save helpers
    // ------------------------------------------------------------
    void loadAppearance();
    void saveAppearance();

    void loadFonts();
    void saveFonts();

    void loadToolbar();
    void saveToolbar();

    void loadThumbnails();
    void saveThumbnails();

    void loadLanguage();
    void saveLanguage();

    // ------------------------------------------------------------
    // Localization
    // ------------------------------------------------------------
    // Standard entry point for updating all translatable texts
    void retranslate();

    // Rebuild texts created in code (combo items, preview labels, etc.)
    void rebuildStaticUiTexts();

    // ------------------------------------------------------------
    // Preview orchestration
    // ------------------------------------------------------------
    void applyPreview();
    void applyPreviewFonts();
    void applyPreviewToolbar();
    void applyPreviewThumbnails();
    void applyPreviewQss();

    // ------------------------------------------------------------
    // Toolbar preview helpers
    // ------------------------------------------------------------
    void setupPreviewIcons();
    void setupPreviewToolbarText();

    // ------------------------------------------------------------
    // Font propagation helpers
    // ------------------------------------------------------------
    void applyFontRecursive(QWidget *root, const QFont &font);

    // ------------------------------------------------------------
    // Sample-based preview content
    // ------------------------------------------------------------
    QString resolveSampleDir() const;
    QStringList buildSampleImagePaths() const;

    void ensureSampleContentLoaded();
    void rebuildThumbnailItems(int thumbSizePx);
    void updateDocumentPreviewPixmap();

    void loadSamplePages();
    void updatePreviewForPage(int index);

    // Cached originals for fast rescaling
    QPixmap m_docPreviewOriginal;
    QList<QPixmap> m_thumbOriginals;
    bool m_sampleContentReady = false;

    // ------------------------------------------------------------
    // Combo rebuild helpers (must preserve current selection)
    // ------------------------------------------------------------
    void rebuildThemeModeComboKeepSelection();
    void rebuildLanguageComboKeepSelection();



    // interfacepane_preview.cpp
    void applyButtonFont(QAbstractButton *btn, const QFont &font);

    // interfacepane_samples.cpp
    QString sampleBasePath() const;

    // interfacepane_qss.cpp
    QString loadBuiltinQss(const QString &name) const;

};

#endif // INTERFACEPANE_H
