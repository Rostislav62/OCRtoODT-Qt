// ============================================================
//  OCRtoODT — Interface Settings Pane (Sample Content Preview)
//  File: settings/interfacepane_samples.cpp
//
//  Responsibility:
//      Implements all preview logic related to real sample content:
//
//          - Locating sample resources on disk or in qrc
//          - Loading sample page images (thumbnails + document preview)
//          - Loading sample OCR text results
//          - Rebuilding thumbnails when size changes
//
//      Design rules:
//          - Preview-only logic
//          - No config writes
//          - No interaction with real OCR pipeline
// ============================================================

#include "interfacepane.h"
#include "ui_interfacepane.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QPixmap>
#include <QTextStream>

// ------------------------------------------------------------
// Sample directory resolution (runtime, disk-based)
// ------------------------------------------------------------

// Tries to locate "resources/sample" directory by walking up
// from executable or current working directory.
QString InterfaceSettingsPane::resolveSampleDir() const
{
    const QStringList anchors = {
        QCoreApplication::applicationDirPath(),
        QDir::currentPath()
    };

    for (const QString &anchor : anchors)
    {
        QDir d(anchor);
        for (int up = 0; up < 8; ++up)
        {
            const QString candidate =
                d.absoluteFilePath("resources/sample");

            if (QDir(candidate).exists())
                return QDir(candidate).absolutePath();

            if (!d.cdUp())
                break;
        }
    }

    return QString();
}

// Builds absolute paths to sample images (0001.png .. 0003.png).
QStringList InterfaceSettingsPane::buildSampleImagePaths() const
{
    const QString sampleDir = resolveSampleDir();
    if (sampleDir.isEmpty())
        return {};

    return {
        QDir(sampleDir).absoluteFilePath("0001.png"),
        QDir(sampleDir).absoluteFilePath("0002.png"),
        QDir(sampleDir).absoluteFilePath("0003.png")
    };
}

// ------------------------------------------------------------
// Sample content initialization
// ------------------------------------------------------------

// Loads and caches original sample images for fast rescaling.
// Also prepares initial thumbnail list items.
void InterfaceSettingsPane::ensureSampleContentLoaded()
{
    if (m_sampleContentReady)
        return;

    const QStringList paths = buildSampleImagePaths();
    if (paths.size() != 3)
        return;

    m_thumbOriginals.clear();
    m_docPreviewOriginal = QPixmap();

    for (int i = 0; i < paths.size(); ++i)
    {
        const QString &path = paths[i];
        if (!QFileInfo::exists(path))
            return;

        QPixmap pix(path);
        if (pix.isNull())
            return;

        m_thumbOriginals.push_back(pix);

        // First page is used as initial document preview.
        if (i == 0)
            m_docPreviewOriginal = pix;
    }

    if (m_docPreviewOriginal.isNull() ||
        m_thumbOriginals.size() != 3)
        return;

    ui->listPreviewPages->clear();

    for (int i = 0; i < 3; ++i)
    {
        auto *item =
            new QListWidgetItem(tr("Page %1").arg(i + 1));
        ui->listPreviewPages->addItem(item);
    }

    ui->lblDocumentPlaceholder->setAlignment(Qt::AlignCenter);
    ui->lblDocumentPlaceholder->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding);

    m_sampleContentReady = true;
}

// ------------------------------------------------------------
// Thumbnail rebuilding
// ------------------------------------------------------------

// Recreates thumbnail icons using cached originals
// according to current thumbnail size.
void InterfaceSettingsPane::rebuildThumbnailItems(int thumbSizePx)
{
    if (!m_sampleContentReady)
        return;

    const int w = thumbSizePx;
    const int h = int(thumbSizePx * 1.414); // A4 portrait ratio

    if (ui->listPreviewPages->count() != 3)
    {
        ui->listPreviewPages->clear();
        for (int i = 0; i < 3; ++i)
        {
            auto *item =
                new QListWidgetItem(tr("Page %1").arg(i + 1));
            ui->listPreviewPages->addItem(item);
        }
    }

    for (int i = 0; i < 3; ++i)
    {
        QListWidgetItem *item =
            ui->listPreviewPages->item(i);

        if (!item)
            continue;

        const QPixmap &src = m_thumbOriginals[i];

        const QPixmap scaled =
            src.scaled(
                QSize(w, h),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation);

        item->setIcon(QIcon(scaled));
        item->setSizeHint(QSize(w + 16, h + 16));
    }
}

// ------------------------------------------------------------
// Document preview scaling
// ------------------------------------------------------------

// Scales current document preview image to fit preview label.
void InterfaceSettingsPane::updateDocumentPreviewPixmap()
{
    if (m_docPreviewOriginal.isNull())
        return;

    const QSize target =
        ui->lblDocumentPlaceholder->contentsRect().size();

    if (target.width() < 10 || target.height() < 10)
        return;

    const QPixmap scaled =
        m_docPreviewOriginal.scaled(
            target,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);

    ui->lblDocumentPlaceholder->setPixmap(scaled);
}

// ------------------------------------------------------------
// Sample OCR/text preview (qrc-based)
// ------------------------------------------------------------

// Loads sample pages (images + text) from qrc resources.
void InterfaceSettingsPane::loadSamplePages()
{
    ui->listPreviewPages->clear();

    const QString base = sampleBasePath();

    for (int i = 1; i <= 3; ++i)
    {
        const QString id =
            QString("%1").arg(i, 4, 10, QLatin1Char('0'));

        const QString imgPath = base + "/" + id + ".png";

        QPixmap pix(imgPath);
        if (pix.isNull())
            continue;

        auto *item = new QListWidgetItem;
        item->setText(tr("Page %1").arg(i));
        item->setIcon(QIcon(pix));
        item->setData(Qt::UserRole, id);

        ui->listPreviewPages->addItem(item);
    }

    if (ui->listPreviewPages->count() > 0)
    {
        ui->listPreviewPages->setCurrentRow(0);
        updatePreviewForPage(0);
    }
}

// Returns base qrc path for sample preview content.
QString InterfaceSettingsPane::sampleBasePath() const
{
    return QString(":/sample/sample");
}

// Slot: reacts to page selection change in preview list.
void InterfaceSettingsPane::onPreviewPageSelected(int row)
{
    if (row < 0)
        return;

    updatePreviewForPage(row);
}

// Updates document image and OCR text preview
// according to selected sample page.
void InterfaceSettingsPane::updatePreviewForPage(int index)
{
    QListWidgetItem *item =
        ui->listPreviewPages->item(index);

    if (!item)
        return;

    const QString id =
        item->data(Qt::UserRole).toString();

    const QString base = sampleBasePath();

    // Update document image preview
    const QString imgPath = base + "/" + id + ".png";
    m_docPreviewOriginal.load(imgPath);
    updateDocumentPreviewPixmap();

    // Update OCR text preview
    const QString txtPath = base + "/" + id + ".txt";
    QFile file(txtPath);

    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream ts(&file);
        ui->editPreviewResult->setPlainText(ts.readAll());
    }
    else
    {
        ui->editPreviewResult->clear();
    }
}
