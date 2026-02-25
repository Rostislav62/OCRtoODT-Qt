// ============================================================
//  OCRtoODT — Interface Settings Pane (Preview Logic)
//  File: settings/interfacepane_preview.cpp
//
//  Responsibility:
//      Implements all UI preview logic for InterfaceSettingsPane,
//      excluding QSS/theme loading and sample content IO.
//
//      Covered preview subsystems:
//          - Font preview and recursive propagation
//          - Toolbar style and icon size preview
//          - Thumbnail size preview (visual only)
//          - Static preview icons and texts
//
//      Design rules:
//          - Preview-only: MUST NOT affect real MainWindow
//          - No file IO, no config access
//          - All effects are local to preview widgets
// ============================================================

#include "interfacepane.h"
#include "ui_interfacepane.h"
#include "core/ConfigManager.h"

#include <QAbstractButton>
#include <QFont>
#include <QToolButton>
#include <QListWidgetItem>
#include <QSizePolicy>

// ------------------------------------------------------------
// Preview orchestration
// ------------------------------------------------------------

// Applies all preview subsystems in correct order.
void InterfaceSettingsPane::applyPreview()
{
    applyPreviewQss();        // visual theme / stylesheet (preview-only)
    applyPreviewFonts();      // font family and size propagation
    applyPreviewToolbar();    // toolbar style + icon size simulation
    applyPreviewThumbnails(); // thumbnail size and layout update
}

// ------------------------------------------------------------
// Font preview
// ------------------------------------------------------------

// Applies selected font family and size to all preview widgets.
// Applies selected font family and size to preview subtree (live)
// ------------------------------------------------------------
// Font preview (QSS-based, consistent with ThemeManager)
// ------------------------------------------------------------
void InterfaceSettingsPane::applyPreviewFonts()
{
    QFont font = ui->fontComboApp->currentFont();
    font.setPointSize(ui->spinAppFontSize->value());

    QString fontQss = QString(
                          "QWidget { font-family: \"%1\"; font-size: %2pt; } "
                          "QPlainTextEdit, QTextEdit, QLineEdit { font-size: %2pt; }")
                          .arg(font.family())
                          .arg(font.pointSize());

    // IMPORTANT:
    // We append font QSS to previewRoot stylesheet
    // instead of using setFont(), because global ThemeManager
    // enforces font via QWidget{} rule.
    QString baseQss = ui->previewRoot->styleSheet();

    // Remove previous font override if exists
    QRegularExpression re("QWidget \\{ font-family:.*?\\}");
    baseQss.remove(re);

    ui->previewRoot->setStyleSheet(baseQss + "\n" + fontQss);
}

// Recursively applies font to a widget subtree (preview only).
// Recursively applies font to entire widget subtree (preview only).
void InterfaceSettingsPane::applyFontRecursive(QWidget *root, const QFont &font)
{
    if (!root)
        return;

    root->setFont(font);

    const auto children = root->findChildren<QWidget*>(
        QString(),
        Qt::FindChildrenRecursively);

    for (QWidget *child : children)
        child->setFont(font);
}

// Applies font to a single abstract button (utility helper).
void InterfaceSettingsPane::applyButtonFont(QAbstractButton *btn, const QFont &font)
{
    if (!btn)
        return;

    btn->setFont(font);
}

// ------------------------------------------------------------
// Toolbar preview (icons / text / both + icon size)
// ------------------------------------------------------------

// Simulates toolbar appearance according to selected settings.
void InterfaceSettingsPane::applyPreviewToolbar()
{
    const int size = ui->spinToolbarIconSize->value();
    const QSize iconSize(size, size);

    QList<QToolButton*> buttons = {
        ui->btnPrevOpen,
        ui->btnPrevClear,
        ui->btnPrevSettings,
        ui->btnPrevRun,
        ui->btnPrevStop,
        ui->btnPrevExport,
        ui->btnPrevAbout,
        ui->btnPrevHelp
    };

    Qt::ToolButtonStyle style = Qt::ToolButtonTextUnderIcon;

    if (ui->radioToolbarIcons->isChecked())
        style = Qt::ToolButtonIconOnly;
    else if (ui->radioToolbarText->isChecked())
        style = Qt::ToolButtonTextOnly;

    for (QToolButton *button : buttons)
    {
        if (!button)
            continue;

        button->setToolButtonStyle(style);
        button->setIconSize(iconSize);
        button->setMinimumWidth(42);

        // Ensure "TextUnderIcon" is never vertically clipped
        button->setMinimumHeight(56);
        button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
}

// ------------------------------------------------------------
// Thumbnail preview (visual only)
// ------------------------------------------------------------

// Updates thumbnail list layout and icon size in preview.
void InterfaceSettingsPane::applyPreviewThumbnails()
{
    if (!m_sampleContentReady)
        return;

    int size = ui->spinThumbSize->value();

    if (size < 100) size = 100;
    if (size > 200) size = 200;

    // A4 portrait aspect ratio approximation
    const QSize iconSize(size, int(size * 1.414));

    ui->listPreviewPages->setIconSize(iconSize);

    for (int i = 0; i < ui->listPreviewPages->count(); ++i)
    {
        QListWidgetItem *item = ui->listPreviewPages->item(i);
        if (!item)
            continue;

        item->setSizeHint(QSize(
            iconSize.width() + 16,
            iconSize.height() + 16));
    }

    rebuildThumbnailItems(size);
}

// ------------------------------------------------------------
// Static preview icons and texts
// ------------------------------------------------------------

// Assigns static icons to preview toolbar buttons.
void InterfaceSettingsPane::setupPreviewIcons()
{
    ui->btnPrevOpen->setIcon(QIcon(":/icons/icons/open.svg"));
    ui->btnPrevClear->setIcon(QIcon(":/icons/icons/clear.svg"));
    ui->btnPrevSettings->setIcon(QIcon(":/icons/icons/settings.svg"));
    ui->btnPrevRun->setIcon(QIcon(":/icons/icons/run.svg"));
    ui->btnPrevStop->setIcon(QIcon(":/icons/icons/stop.svg"));
    ui->btnPrevExport->setIcon(QIcon(":/icons/icons/export.svg"));
    ui->btnPrevAbout->setIcon(QIcon(":/icons/icons/about.svg"));
    ui->btnPrevHelp->setIcon(QIcon(":/icons/icons/help.svg"));
}

// Assigns translated texts and tooltips to preview toolbar buttons.
void InterfaceSettingsPane::setupPreviewToolbarText()
{
    ui->btnPrevOpen->setText(tr("Open"));
    ui->btnPrevClear->setText(tr("Clear"));
    ui->btnPrevSettings->setText(tr("Settings"));
    ui->btnPrevRun->setText(tr("Run"));
    ui->btnPrevStop->setText(tr("Stop"));
    ui->btnPrevExport->setText(tr("Export"));
    ui->btnPrevAbout->setText(tr("About"));
    ui->btnPrevHelp->setText(tr("Help"));

    ui->btnPrevOpen->setToolTip(ui->btnPrevOpen->text());
    ui->btnPrevClear->setToolTip(ui->btnPrevClear->text());
    ui->btnPrevSettings->setToolTip(ui->btnPrevSettings->text());
    ui->btnPrevRun->setToolTip(ui->btnPrevRun->text());
    ui->btnPrevStop->setToolTip(ui->btnPrevStop->text());
    ui->btnPrevExport->setToolTip(ui->btnPrevExport->text());
    ui->btnPrevAbout->setToolTip(ui->btnPrevAbout->text());
    ui->btnPrevHelp->setToolTip(ui->btnPrevHelp->text());
}


// ------------------------------------------------------------
// Appearance / Fonts / Toolbar / Thumbnails
// (Dispatcher-level load/save stubs)
// ------------------------------------------------------------

// Appearance preview is driven by QSS + ThemeManager.
// Initial state is already reflected via applyPreview().
void InterfaceSettingsPane::loadAppearance()
{
    ConfigManager &cfg = ConfigManager::instance();

    QString mode = cfg.get("ui.theme_mode", "dark").toString();

    int idx = 1; // default: Dark
    if (mode == "light") idx = 0;
    else if (mode == "dark") idx = 1;
    else if (mode == "auto") idx = 2;
    else if (mode == "system") idx = 2;

    ui->comboThemeMode->setCurrentIndex(idx);
    ui->editCustomQssPath->setText(cfg.get("ui.custom_qss", "").toString());
}

// Appearance settings are persisted via ThemeManager
// and related panes; nothing to commit here yet.
void InterfaceSettingsPane::saveAppearance()
{
    ConfigManager &cfg = ConfigManager::instance();

    QString mode = "dark";
    switch (ui->comboThemeMode->currentIndex())
    {
    case 0: mode = "light"; break;
    case 1: mode = "dark";  break;
    case 2: mode = "auto";  break;
    default: break;
    }

    cfg.set("ui.theme_mode", mode);
    cfg.set("ui.custom_qss", ui->editCustomQssPath->text());
}

    // Font UI widgets are already synchronized via preview logic.
void InterfaceSettingsPane::loadFonts()
{
    ConfigManager &cfg = ConfigManager::instance();

    QString family = cfg.get("ui.app_font_family", "").toString();
    int size = cfg.get("ui.app_font_size", 11).toInt();

    if (!family.isEmpty())
        ui->fontComboApp->setCurrentFont(QFont(family));

    ui->spinAppFontSize->setValue(size);
}

    // Font persistence handled at application level.
void InterfaceSettingsPane::saveFonts()
{
    ConfigManager &cfg = ConfigManager::instance();
    cfg.set("ui.app_font_family", ui->fontComboApp->currentFont().family());
    cfg.set("ui.app_font_size", ui->spinAppFontSize->value());
}

    // Toolbar preview is local and stateless.
void InterfaceSettingsPane::loadToolbar()
{
    ConfigManager &cfg = ConfigManager::instance();

    QString style = cfg.get("ui.toolbar_style", "icons_text").toString();
    if (style == "icons")
        ui->radioToolbarIcons->setChecked(true);
    else if (style == "text")
        ui->radioToolbarText->setChecked(true);
    else
        ui->radioToolbarBoth->setChecked(true);

    ui->spinToolbarIconSize->setValue(
        cfg.get("ui.toolbar_icon_size", 24).toInt());
}

    // Toolbar settings are applied globally elsewhere.
void InterfaceSettingsPane::saveToolbar()
{
    ConfigManager &cfg = ConfigManager::instance();

    QString style = "icons_text";
    if (ui->radioToolbarIcons->isChecked())
        style = "icons";
    else if (ui->radioToolbarText->isChecked())
        style = "text";

    cfg.set("ui.toolbar_style", style);
    cfg.set("ui.toolbar_icon_size", ui->spinToolbarIconSize->value());
}

    // Thumbnail preview initializes itself on demand.
void InterfaceSettingsPane::loadThumbnails()
{
    ConfigManager &cfg = ConfigManager::instance();
    ui->spinThumbSize->setValue(
        cfg.get("ui.thumbnail_size", 160).toInt());
}

    // Thumbnail size is preview-only at this stage.
void InterfaceSettingsPane::saveThumbnails()
{
    ConfigManager &cfg = ConfigManager::instance();

    int s = ui->spinThumbSize->value();
    if (s < 100) s = 100;
    if (s > 200) s = 200;

    cfg.set("ui.thumbnail_size", s);
}

