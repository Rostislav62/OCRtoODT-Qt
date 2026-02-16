// ============================================================
//  OCRtoODT — Interface Settings Pane (QSS Preview Handling)
//  File: settings/interfacepane_qss.cpp
//
//  Responsibility:
//      Implements theme/QSS preview logic for InterfaceSettingsPane:
//
//          - Built-in light/dark theme preview
//          - Custom QSS loading from file
//          - Token resolution inside QSS (@color variables)
//          - Applying resulting stylesheet to preview root only
//
//      Design rules:
//          - Preview-only (NO global QApplication styling)
//          - No writes to config.yaml
//          - No interaction with ThemeManager runtime state
// ============================================================

#include "interfacepane.h"
#include "ui_interfacepane.h"

#include <QFile>
#include <QMap>
#include <QStringList>

// ------------------------------------------------------------
// QSS preview entry point
// ------------------------------------------------------------

// Applies currently selected theme/QSS to preview root widget only.
void InterfaceSettingsPane::applyPreviewQss()
{
    const int mode = ui->comboThemeMode->currentIndex();
    const QString customPath =
        ui->editCustomQssPath->text().trimmed();

    QString rawQss;

    // 0 = Light, 1 = Dark, 2 = Custom
    if (mode == 2)
    {
        // Custom QSS
        if (customPath.isEmpty())
        {
            ui->previewRoot->setStyleSheet(QString());
            return;
        }

        QFile f(customPath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            ui->previewRoot->setStyleSheet(QString());
            return;
        }

        rawQss = QString::fromUtf8(f.readAll());
    }
    else
    {
        // Built-in theme
        rawQss = (mode == 0)
                     ? loadBuiltinQss("light")
                     : loadBuiltinQss("dark");
    }

    ui->previewRoot->setStyleSheet(rawQss);
}


// ------------------------------------------------------------
// QSS token resolver
// ------------------------------------------------------------

// Resolves @token: value; definitions inside QSS and replaces
// all occurrences of @token with resolved values.
static QString resolveTokenizedQss(const QString &input)
{
    QMap<QString, QString> tokens;

    const QStringList lines = input.split('\n');
    QStringList output;
    output.reserve(lines.size());

    for (const QString &line : lines)
    {
        const QString t = line.trimmed();

        // Preserve comments and empty lines
        if (t.isEmpty() ||
            t.startsWith("/*") ||
            t.startsWith("*") ||
            t.startsWith("*/"))
        {
            output.push_back(line);
            continue;
        }

        // Token definition: @name: value;
        if (t.startsWith('@'))
        {
            const int colon = t.indexOf(':');
            const int semi  = t.lastIndexOf(';');

            if (colon > 1 && semi > colon)
            {
                const QString key =
                    t.mid(1, colon - 1).trimmed();

                const QString value =
                    t.mid(colon + 1, semi - colon - 1).trimmed();

                if (!key.isEmpty() && !value.isEmpty())
                {
                    tokens.insert(key, value);
                    continue; // do not emit token definition line
                }
            }
        }

        output.push_back(line);
    }

    QString result = output.join('\n');

    // Replace all tokens
    for (auto it = tokens.constBegin(); it != tokens.constEnd(); ++it)
        result.replace(QString("@%1").arg(it.key()), it.value());

    return result;
}

// ------------------------------------------------------------
// Built-in QSS loading
// ------------------------------------------------------------

// Loads built-in QSS (light/dark) from resources and resolves tokens.
QString InterfaceSettingsPane::loadBuiltinQss(const QString &name) const
{
    const QString resPath =
        QString(":/themes/themes/%1.qss").arg(name);

    QFile f(resPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString();

    const QString raw = QString::fromUtf8(f.readAll());
    if (raw.isEmpty())
        return QString();

    return resolveTokenizedQss(raw);
}
