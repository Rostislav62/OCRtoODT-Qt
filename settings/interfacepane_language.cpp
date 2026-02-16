// ============================================================
//  OCRtoODT — Interface Settings Pane (Language Preview Handling)
//  File: settings/interfacepane_language.cpp
//
//  Responsibility:
//      Implements language-related logic for InterfaceSettingsPane:
//
//          - Load/save interface language from config.yaml
//          - Populate language selection combo box
//          - Live preview of UI language inside settings dialog
//          - Translator install/remove strictly for preview scope
//
//      Design rules:
//          - Preview language changes MUST NOT affect MainWindow
//          - Translator is installed temporarily and locally
//          - No global language switch happens here
//
//      Architectural note:
//          This file intentionally does NOT depend on ThemeManager
//          language application logic. This allows future extraction
//          of language handling into a dedicated LanguageManager.
// ============================================================


// ============================================================
//  OCRtoODT — Interface Settings Pane (Language)
//  File: settings/interfacepane_language.cpp
//
//  Responsibility:
//      All language + localization logic for InterfaceSettingsPane.
//      Uses global LanguageManager (single QTranslator).
// ============================================================

#include "interfacepane.h"
#include "ui_interfacepane.h"

#include "core/ConfigManager.h"
#include "core/LanguageManager.h"

#include <QSignalBlocker>

void InterfaceSettingsPane::loadLanguage()
{
    const QString current =
        LanguageManager::instance()->currentLanguage().trimmed().isEmpty()
            ? ConfigManager::instance().get("ui.language", "en").toString().trimmed()
            : LanguageManager::instance()->currentLanguage().trimmed();

    const QSignalBlocker b(ui->comboLanguage);
    const int idx = ui->comboLanguage->findData(current);
    if (idx >= 0)
        ui->comboLanguage->setCurrentIndex(idx);
}

void InterfaceSettingsPane::saveLanguage()
{
    const QString code = ui->comboLanguage->currentData().toString().trimmed();
    ConfigManager::instance().set("ui.language",
                                  code.isEmpty() ? QString("en") : code);
}

void InterfaceSettingsPane::onLanguageComboChanged(int)
{
    const QString code = ui->comboLanguage->currentData().toString().trimmed();
    if (!code.isEmpty())
        LanguageManager::instance()->setLanguage(code);
}

void InterfaceSettingsPane::onGlobalLanguageChanged(const QString &)
{
    retranslate();
    applyPreview();
}

void InterfaceSettingsPane::retranslate()
{
    // 1) Designer-generated texts
    ui->retranslateUi(this);

    // 2) Texts created in code
    rebuildStaticUiTexts();

    // 3) Preview toolbar texts (created manually in code)
    setupPreviewToolbarText();
}


void InterfaceSettingsPane::rebuildStaticUiTexts()
{
    rebuildThemeModeComboKeepSelection();
    rebuildLanguageComboKeepSelection();

    // Update sample page labels
    if (ui->listPreviewPages)
    {
        for (int i = 0; i < ui->listPreviewPages->count(); ++i)
        {
            if (auto *it = ui->listPreviewPages->item(i))
                it->setText(tr("Page %1").arg(i + 1));
        }
    }
}


void InterfaceSettingsPane::rebuildLanguageComboKeepSelection()
{
    const QString currentCode = ui->comboLanguage->currentData().toString();
    const QSignalBlocker b(ui->comboLanguage);

    ui->comboLanguage->clear();
    ui->comboLanguage->addItem(tr("English"),  "en");
    ui->comboLanguage->addItem(tr("Russian"),  "ru");
    ui->comboLanguage->addItem(tr("Romanian"), "ro");

    const int idx = ui->comboLanguage->findData(currentCode);
    if (idx >= 0)
        ui->comboLanguage->setCurrentIndex(idx);
}
