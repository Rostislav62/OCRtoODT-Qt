// ============================================================
//  OCRtoODT — ODT Settings Pane
//  File: settings/odtpane.cpp
//
//  Responsibility:
//      Implements ODT Formatting panel logic.
//
//  Architecture:
//      - ConfigManager = persistence
//      - OdtLayoutModel = preview truth
//      - UI contains only mapping logic
// ============================================================

#include "odtpane.h"
#include "ui_odtpane.h"

#include "core/ConfigManager.h"
#include "core/LanguageManager.h"
#include "core/layout/OdtLayoutModel.h"

#include <QButtonGroup>

// ============================================================
// Constructor
// ============================================================

ODTSettingsPane::ODTSettingsPane(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ODTSettingsPane)
{
    ui->setupUi(this);

    // --------------------------------------------------------
    // Stable combo itemData (must NOT depend on translated text)
    // --------------------------------------------------------
    auto initComboData = [this]()
    {
        // Paper size: stable key in data
        // (texts in UI remain translatable if you ever translate them)
        if (ui->comboPaperSize->count() >= 3)
        {
            ui->comboPaperSize->setItemData(0, "A4");
            ui->comboPaperSize->setItemData(1, "Letter");
            ui->comboPaperSize->setItemData(2, "Legal");
        }

        // Paragraph spacing: pt presets; Custom = -1
        // Order in .ui: 0pt, 3pt, 6pt, 12pt, Custom…
        if (ui->comboParagraphSpacing->count() >= 5)
        {
            ui->comboParagraphSpacing->setItemData(0, 0.0);
            ui->comboParagraphSpacing->setItemData(1, 3.0);
            ui->comboParagraphSpacing->setItemData(2, 6.0);
            ui->comboParagraphSpacing->setItemData(3, 12.0);
            ui->comboParagraphSpacing->setItemData(4, -1.0);
        }

        // Line height preset: percent presets; Custom = -1
        // Order in .ui: Single, 1.5, Double, Custom…
        if (ui->comboLineHeightPreset->count() >= 4)
        {
            ui->comboLineHeightPreset->setItemData(0, 100);
            ui->comboLineHeightPreset->setItemData(1, 150);
            ui->comboLineHeightPreset->setItemData(2, 200);
            ui->comboLineHeightPreset->setItemData(3, -1);
        }
    };

    initComboData();


    // --------------------------------------------------------
    // Alignment exclusive group
    // --------------------------------------------------------
    QButtonGroup *alignGroup = new QButtonGroup(this);
    alignGroup->setExclusive(true);

    alignGroup->addButton(ui->btnAlignLeft);
    alignGroup->addButton(ui->btnAlignCenter);
    alignGroup->addButton(ui->btnAlignRight);
    alignGroup->addButton(ui->btnAlignJustify);

    // --------------------------------------------------------
    // Paragraph spacing preset logic
    // --------------------------------------------------------
    connect(ui->comboParagraphSpacing,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this]()
            {
                const double data = ui->comboParagraphSpacing->currentData().toDouble();
                const bool custom = (data < 0.0);

                ui->spinParagraphSpacingCustom->setEnabled(custom);
                updatePagePreview();
            });


    connect(ui->spinParagraphSpacingCustom,
            qOverload<double>(&QDoubleSpinBox::valueChanged),
            this,
            &ODTSettingsPane::updatePagePreview);

    // --------------------------------------------------------
    // Line height preset logic
    // --------------------------------------------------------
    connect(ui->comboLineHeightPreset,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this]()
            {
                const int data = ui->comboLineHeightPreset->currentData().toInt();
                const bool custom = (data < 0);

                ui->spinLineHeightPercent->setEnabled(custom);
                updatePagePreview();
            });


    connect(ui->spinLineHeightPercent,
            qOverload<int>(&QSpinBox::valueChanged),
            this,
            &ODTSettingsPane::updatePagePreview);

    // --------------------------------------------------------
    // Live preview connections
    // --------------------------------------------------------
    connect(ui->comboFontFamily, &QFontComboBox::currentFontChanged,
            this, &ODTSettingsPane::updatePagePreview);

    connect(ui->spinFontSizePt, qOverload<int>(&QSpinBox::valueChanged),
            this, &ODTSettingsPane::updatePagePreview);

    connect(ui->spinFirstLineIndent, qOverload<double>(&QDoubleSpinBox::valueChanged),
            this, &ODTSettingsPane::updatePagePreview);

    connect(ui->spinMarginTop, qOverload<int>(&QSpinBox::valueChanged),
            this, &ODTSettingsPane::updatePagePreview);

    connect(ui->spinMarginBottom, qOverload<int>(&QSpinBox::valueChanged),
            this, &ODTSettingsPane::updatePagePreview);

    connect(ui->spinMarginLeft, qOverload<int>(&QSpinBox::valueChanged),
            this, &ODTSettingsPane::updatePagePreview);

    connect(ui->spinMarginRight, qOverload<int>(&QSpinBox::valueChanged),
            this, &ODTSettingsPane::updatePagePreview);

    connect(ui->comboPaperSize,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &ODTSettingsPane::updatePagePreview);

    connect(ui->btnAlignLeft,    &QToolButton::clicked, this, &ODTSettingsPane::updatePagePreview);
    connect(ui->btnAlignCenter,  &QToolButton::clicked, this, &ODTSettingsPane::updatePagePreview);
    connect(ui->btnAlignRight,   &QToolButton::clicked, this, &ODTSettingsPane::updatePagePreview);
    connect(ui->btnAlignJustify, &QToolButton::clicked, this, &ODTSettingsPane::updatePagePreview);

    connect(ui->spinMaxEmpty, qOverload<int>(&QSpinBox::valueChanged),
            this, &ODTSettingsPane::updatePagePreview);

    connect(ui->chkPageBreak, &QCheckBox::toggled,
            this, &ODTSettingsPane::updatePagePreview);

    connect(LanguageManager::instance(),
            &LanguageManager::languageChanged,
            this,
            &ODTSettingsPane::retranslate);


    connect(ui->btnRestoreDefaults,
            &QPushButton::clicked,
            this,
            [this]()
            {
                ConfigManager &cfg = ConfigManager::instance();

                cfg.set("odt.paper_size", "A4");
                cfg.set("odt.font_name", "Times New Roman");
                cfg.set("odt.font_size_pt", 12);

                cfg.set("odt.margin_top_mm", 20);
                cfg.set("odt.margin_bottom_mm", 15);
                cfg.set("odt.margin_left_mm", 20);
                cfg.set("odt.margin_right_mm", 15);

                cfg.set("odt.text_align", "justify");
                cfg.set("odt.first_line_indent_mm", 10);
                cfg.set("odt.paragraph_spacing_after_pt", 6);
                cfg.set("odt.line_height_percent", 120);
                cfg.set("odt.max_empty_lines", 1);
                cfg.set("odt.page_break", true);

                loadODT();
                updatePagePreview();
            });

}

// ============================================================
// Destructor
// ============================================================

ODTSettingsPane::~ODTSettingsPane()
{
    delete ui;
}

// ============================================================
// Public API
// ============================================================

void ODTSettingsPane::load()
{
    loadODT();
    updatePagePreview();
}

void ODTSettingsPane::save()
{
    saveODT();
}

// ============================================================
// Load
// ============================================================

void ODTSettingsPane::loadODT()
{
    ConfigManager &cfg = ConfigManager::instance();

    {
        const QString key = cfg.get("odt.paper_size", "A4").toString();
        const int idx = ui->comboPaperSize->findData(key);
        if (idx >= 0)
            ui->comboPaperSize->setCurrentIndex(idx);
        else
            ui->comboPaperSize->setCurrentIndex(0); // A4 fallback
    }


    ui->comboFontFamily->setCurrentFont(
        QFont(cfg.get("odt.font_name", "Times New Roman").toString()));

    ui->spinFontSizePt->setValue(
        cfg.get("odt.font_size_pt", 12).toInt());

    ui->spinMarginTop->setValue(cfg.get("odt.margin_top_mm", 20).toInt());
    ui->spinMarginBottom->setValue(cfg.get("odt.margin_bottom_mm", 15).toInt());
    ui->spinMarginLeft->setValue(cfg.get("odt.margin_left_mm", 20).toInt());
    ui->spinMarginRight->setValue(cfg.get("odt.margin_right_mm", 15).toInt());

    applyAlignmentFromConfig(
        cfg.get("odt.text_align", "justify").toString());

    ui->spinFirstLineIndent->setValue(
        cfg.get("odt.first_line_indent_mm", 10).toDouble());

    // Paragraph spacing
    {
        const double spacing = cfg.get("odt.paragraph_spacing_after_pt", 6).toDouble();

        const int idx = ui->comboParagraphSpacing->findData(spacing);
        if (idx >= 0)
        {
            ui->comboParagraphSpacing->setCurrentIndex(idx);
            ui->spinParagraphSpacingCustom->setEnabled(false);
        }
        else
        {
            const int customIdx = ui->comboParagraphSpacing->findData(-1.0);
            if (customIdx >= 0)
                ui->comboParagraphSpacing->setCurrentIndex(customIdx);

            ui->spinParagraphSpacingCustom->setEnabled(true);
            ui->spinParagraphSpacingCustom->setValue(spacing);
        }
    }


    // Line height
    {
        const int lh = cfg.get("odt.line_height_percent", 120).toInt();

        const int idx = ui->comboLineHeightPreset->findData(lh);
        if (idx >= 0)
        {
            ui->comboLineHeightPreset->setCurrentIndex(idx);
            ui->spinLineHeightPercent->setEnabled(false);
        }
        else
        {
            const int customIdx = ui->comboLineHeightPreset->findData(-1);
            if (customIdx >= 0)
                ui->comboLineHeightPreset->setCurrentIndex(customIdx);

            ui->spinLineHeightPercent->setEnabled(true);
            ui->spinLineHeightPercent->setValue(lh);
        }
    }


    ui->spinMaxEmpty->setValue(
        cfg.get("odt.max_empty_lines", 1).toInt());

    ui->chkPageBreak->setChecked(
        cfg.get("odt.page_break", true).toBool());
}

// ============================================================
// Save
// ============================================================

void ODTSettingsPane::saveODT()
{
    ConfigManager &cfg = ConfigManager::instance();

    cfg.set("odt.paper_size", ui->comboPaperSize->currentData().toString());
    cfg.set("odt.font_name", ui->comboFontFamily->currentFont().family());
    cfg.set("odt.font_size_pt", ui->spinFontSizePt->value());

    cfg.set("odt.margin_top_mm", ui->spinMarginTop->value());
    cfg.set("odt.margin_bottom_mm", ui->spinMarginBottom->value());
    cfg.set("odt.margin_left_mm", ui->spinMarginLeft->value());
    cfg.set("odt.margin_right_mm", ui->spinMarginRight->value());

    cfg.set("odt.text_align", alignmentToString());
    cfg.set("odt.first_line_indent_mm", ui->spinFirstLineIndent->value());

    double spacing = ui->comboParagraphSpacing->currentData().toDouble();
    if (spacing < 0.0)
        spacing = ui->spinParagraphSpacingCustom->value();


    cfg.set("odt.paragraph_spacing_after_pt", spacing);

    int lineHeight = ui->comboLineHeightPreset->currentData().toInt();
    if (lineHeight < 0)
        lineHeight = ui->spinLineHeightPercent->value();


    cfg.set("odt.line_height_percent", lineHeight);

    cfg.set("odt.max_empty_lines", ui->spinMaxEmpty->value());
    cfg.set("odt.page_break", ui->chkPageBreak->isChecked());
}

static PaperFormat paperFormatFromKey(const QString &key)
{
    if (key == "Letter") return PaperFormat::Letter;
    if (key == "Legal")  return PaperFormat::Legal;
    return PaperFormat::A4;
}

// ============================================================
// Preview
// ============================================================

void ODTSettingsPane::updatePagePreview()
{
    OdtLayoutModel model;

    model.setFontName(ui->comboFontFamily->currentFont().family());
    model.setFontSizePt(ui->spinFontSizePt->value());

    model.setMarginTopMM(ui->spinMarginTop->value());
    model.setMarginBottomMM(ui->spinMarginBottom->value());
    model.setMarginLeftMM(ui->spinMarginLeft->value());
    model.setMarginRightMM(ui->spinMarginRight->value());

    model.setFirstLineIndentMM(ui->spinFirstLineIndent->value());
    model.setAlignment(parseAlignmentFromString(alignmentToString()));

    double spacing = ui->comboParagraphSpacing->currentData().toDouble();
    if (spacing < 0.0)
        spacing = ui->spinParagraphSpacingCustom->value();


    model.setParagraphSpacingAfterPt(spacing);

    int lineHeight = ui->comboLineHeightPreset->currentData().toInt();
    if (lineHeight < 0)
        lineHeight = ui->spinLineHeightPercent->value();


    model.setLineHeightPercent(lineHeight);

    model.setMaxEmptyLines(ui->spinMaxEmpty->value());
    model.setPageBreakEnabled(ui->chkPageBreak->isChecked());

    const QString paperKey = ui->comboPaperSize->currentData().toString();
    ui->pageFrame->setPaperFormat(paperFormatFromKey(paperKey));
    ui->pageFrame->setLayoutModel(model);
    ui->pageFrame->update();
}

// ============================================================
// Alignment helpers
// ============================================================

void ODTSettingsPane::applyAlignmentFromConfig(const QString &align)
{
    if (align == "left") ui->btnAlignLeft->setChecked(true);
    else if (align == "center") ui->btnAlignCenter->setChecked(true);
    else if (align == "right") ui->btnAlignRight->setChecked(true);
    else ui->btnAlignJustify->setChecked(true);
}

QString ODTSettingsPane::alignmentToString() const
{
    if (ui->btnAlignLeft->isChecked()) return "left";
    if (ui->btnAlignCenter->isChecked()) return "center";
    if (ui->btnAlignRight->isChecked()) return "right";
    return "justify";
}

Qt::Alignment ODTSettingsPane::parseAlignmentFromString(const QString &a) const
{
    if (a == "left") return Qt::AlignLeft;
    if (a == "center") return Qt::AlignCenter;
    if (a == "right") return Qt::AlignRight;
    return Qt::AlignJustify;
}

// ============================================================
// Localization
// ============================================================

void ODTSettingsPane::retranslate()
{
    ui->retranslateUi(this);
}
