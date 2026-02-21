// ============================================================
//  OCRtoODT — General Settings Pane
//  File: settings/generalpane.cpp
//
//  Responsibility:
//      Implements the "General" tab inside SettingsDialog.
//
//      Execution Strategy:
//          - Automatic  → parallel_enabled=true,  num_processes="auto"
//          - Sequential → parallel_enabled=false, num_processes="1"
//          - Parallel   → parallel_enabled=true,  num_processes="auto"
//
//      Data Execution Mode:
//          - Automatic     → general.mode="auto"
//          - Memory only   → general.mode="ram_only"
//          - Disk assisted → general.mode="disk_only"
//
//      Control Level:
//          - Standard / Professional (ui.expert_mode)
//
//      Config keys used:
//          general.parallel_enabled
//          general.num_processes
//          general.mode
//          ui.expert_mode
//
//  Localization notes:
//      - This file does NOT define user-visible text directly
//      - All translatable strings are defined in generalpane.ui
// ============================================================

#include "generalpane.h"
#include "ui_generalpane.h"

#include "core/ConfigManager.h"
#include "core/LanguageManager.h"
#include "core/LogRouter.h" // CHANGED: unified logging via LogRouter


#include <QStyle>
#include <QToolButton>
#include <QButtonGroup>
#include <QPixmap>


// ============================================================
// Constructor
// ============================================================
GeneralSettingsPane::GeneralSettingsPane(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::GeneralSettingsPane)
{
    ui->setupUi(this);

    // --------------------------------------------------------
    // CPU execution strategy — exclusive toggle group
    // --------------------------------------------------------
    auto *cpuGroup = new QButtonGroup(this);
    cpuGroup->setExclusive(true);

    cpuGroup->addButton(ui->btnCpuAuto);
    cpuGroup->addButton(ui->btnSequential);


    // --------------------------------------------------------
    // Data execution mode — exclusive toggle group
    // --------------------------------------------------------
    auto *dataModeGroup = new QButtonGroup(this);
    dataModeGroup->setExclusive(true);

    dataModeGroup->addButton(ui->btnDataAuto);
    dataModeGroup->addButton(ui->btnDataRam);
    dataModeGroup->addButton(ui->btnDataDisk);

    // --------------------------------------------------------
    // Control level — exclusive toggle group
    // --------------------------------------------------------
    auto *controlLevelGroup = new QButtonGroup(this);
    controlLevelGroup->setExclusive(true);

    controlLevelGroup->addButton(ui->btnModeStandard);
    controlLevelGroup->addButton(ui->btnModePro);





    connect(LanguageManager::instance(),
            &LanguageManager::languageChanged,
            this,
            &GeneralSettingsPane::retranslate);


    // --------------------------------------------------------
    // Mark card-style buttons (used by QSS)
    // --------------------------------------------------------
    const QList<QToolButton*> cardButtons = {
        // Execution strategy
        ui->btnCpuAuto,
        ui->btnSequential,

        // Data execution mode
        ui->btnDataAuto,
        ui->btnDataRam,
        ui->btnDataDisk,

        // Control level
        ui->btnModeStandard,
        ui->btnModePro
    };

    for (QToolButton *btn : cardButtons) {
        btn->setProperty("card", true);

    }


    // --------------------------------------------------------
    // Dynamic hint + icon (CPU mode)
    // --------------------------------------------------------
    auto updateCpuUi = [this]() {
        if (ui->btnSequential->isChecked()) {
            ui->lblProcessingHint->setText(tr("Sequential mode uses fewer resources."));
            ui->lblCpuModeIcon->setPixmap(QPixmap(":/icons/icons/sequential.png"));
        } else {
            ui->lblProcessingHint->setText(tr("Automatic mode is recommended."));
            ui->lblCpuModeIcon->setPixmap(QPixmap(":/icons/icons/auto.png"));
        }
    };

    // --------------------------------------------------------
    // Dynamic hint + icon (Data execution mode)
    // --------------------------------------------------------
    auto updateDataModeUi = [this]() {
        if (ui->btnDataRam->isChecked()) {
            ui->lblDataModeHint->setText(
                tr("All processing steps are executed strictly in memory.")
                );
            ui->lblDataModeIcon->setPixmap(
                QPixmap(":/icons/icons/ram.png")
                );
        } else if (ui->btnDataDisk->isChecked()) {
            ui->lblDataModeHint->setText(
                tr("Disk is used as primary data storage (recommended for large documents).")
                );
            ui->lblDataModeIcon->setPixmap(
                QPixmap(":/icons/icons/disk.png")
                );
        } else {
            ui->lblDataModeHint->setText(
                tr("Program automatically chooses the optimal data execution mode.")
                );
            ui->lblDataModeIcon->setPixmap(
                QPixmap(":/icons/icons/autopc.png")
                );
        }
    };


    // --------------------------------------------------------
    // Dynamic hint + icon (Control level)
    // --------------------------------------------------------
    auto updateControlLevelUi = [this]() {
        if (ui->btnModePro->isChecked()) {
            ui->lblControlLevelHint->setText(
                tr("Professional mode unlocks advanced settings. Changing them may affect OCR quality.")
                );
            ui->lblControlLevelIcon->setPixmap(
                QPixmap(":/icons/icons/pro.png")
                );
        } else {
            ui->lblControlLevelHint->setText(
                tr("Recommended mode with safe default settings.")
                );
            ui->lblControlLevelIcon->setPixmap(
                QPixmap(":/icons/icons/standard.png")
                );
        }
    };



    // Run once at startup
    updateCpuUi();

    // And update on changes
    connect(ui->btnCpuAuto, &QToolButton::toggled, this, [updateCpuUi](bool) { updateCpuUi(); });
    connect(ui->btnSequential, &QToolButton::toggled, this, [updateCpuUi](bool) { updateCpuUi(); });

    // Run once at startup
    updateDataModeUi();

    // Update on changes
    connect(ui->btnDataAuto, &QToolButton::toggled,
            this, [updateDataModeUi](bool) { updateDataModeUi(); });
    connect(ui->btnDataRam, &QToolButton::toggled,
            this, [updateDataModeUi](bool) { updateDataModeUi(); });
    connect(ui->btnDataDisk, &QToolButton::toggled,
            this, [updateDataModeUi](bool) { updateDataModeUi(); });

    // Run once at startup
    updateControlLevelUi();

    // Update on changes
    connect(ui->btnModeStandard, &QToolButton::toggled,
            this, [updateControlLevelUi](bool) { updateControlLevelUi(); });
    connect(ui->btnModePro, &QToolButton::toggled,
            this, [updateControlLevelUi](bool) { updateControlLevelUi(); });


}

GeneralSettingsPane::~GeneralSettingsPane()
{
    delete ui;
}

// ============================================================
// Load values from config.yaml → UI
// ============================================================
void GeneralSettingsPane::load()
{
    ConfigManager &cfg = ConfigManager::instance();

    // --------------------------------------------------------
    // Control level
    // --------------------------------------------------------
    const bool expertMode = cfg.get("ui.expert_mode", false).toBool();
    if (expertMode) {
        ui->btnModePro->setChecked(true);
    } else {
        ui->btnModeStandard->setChecked(true);
    }

    // --------------------------------------------------------
    // Execution strategy
    // --------------------------------------------------------
    const bool parallelEnabled =
        cfg.get("general.parallel_enabled", true).toBool();
    const QString numProcesses =
        cfg.get("general.num_processes", "auto").toString().trimmed();

    ui->btnCpuAuto->setChecked(false);
    ui->btnSequential->setChecked(false);


    if (!parallelEnabled) {
        ui->btnSequential->setChecked(true);
    } else if (numProcesses == "auto") {
        ui->btnCpuAuto->setChecked(true);
    } else {
        ui->btnCpuAuto->setChecked(true);
    }

    // --------------------------------------------------------
    // Data execution mode
    // --------------------------------------------------------
    const QString mode =
        cfg.get("general.mode", "auto").toString().trimmed();

    ui->btnDataAuto->setChecked(false);
    ui->btnDataRam->setChecked(false);
    ui->btnDataDisk->setChecked(false);

    if (mode == "ram_only") {
        ui->btnDataRam->setChecked(true);
    } else if (mode == "disk_only") {
        ui->btnDataDisk->setChecked(true);
    } else {
        ui->btnDataAuto->setChecked(true);
    }
}

// ============================================================
// Save values from UI → ConfigManager
// ============================================================
void GeneralSettingsPane::save()
{
    LogRouter::instance().debug(
        QString("[GeneralSettingsPane::save] btnCpuAuto=%1, btnSequential=%2")
            .arg(ui->btnCpuAuto->isChecked())
            .arg(ui->btnSequential->isChecked())
        );
    LogRouter::instance().debug("[GeneralSettingsPane] save() entered");

    ConfigManager &cfg = ConfigManager::instance();

    // --------------------------------------------------------
    // Control level
    // --------------------------------------------------------
    cfg.set("ui.expert_mode", ui->btnModePro->isChecked());

    // --------------------------------------------------------
    // Execution strategy
    // --------------------------------------------------------
    if (ui->btnCpuAuto->isChecked()) {
        cfg.set("general.parallel_enabled", true);
        cfg.set("general.num_processes", "auto");
    } else if (ui->btnSequential->isChecked()) {
        cfg.set("general.parallel_enabled", false);
        cfg.set("general.num_processes", "1");
    }

    // --------------------------------------------------------
    // Data execution mode
    // --------------------------------------------------------
    if (ui->btnDataRam->isChecked()) {
        cfg.set("general.mode", "ram_only");
    } else if (ui->btnDataDisk->isChecked()) {
        cfg.set("general.mode", "disk_only");
    } else {
        cfg.set("general.mode", "auto");
    }
}


// ============================================================
// Retranslate UI after language change
// ============================================================
void GeneralSettingsPane::retranslate()
{
    ui->retranslateUi(this);
}
