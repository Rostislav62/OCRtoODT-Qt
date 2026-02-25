// ============================================================
//  OCRtoODT — Export Dialog
//  File: dialogs/export.h
//
//  Responsibility:
//      Modal UI to choose export format and output directory,
//      then запускает экспорт ПОСЛЕ accept().
//
//  STRICT RULES:
//      - MainWindow не содержит логики экспорта
//      - Диалог НЕ ищет данные сам
//      - Источник данных передаётся извне (STEP 4 result)
// ============================================================

#ifndef EXPORT_H
#define EXPORT_H

#include <QDialog>
#include <QString>
#include <QVector>

namespace Ui { class ExportDialog; }

namespace Core {
struct VirtualPage;
}

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    // pages — НЕ владеем, просто используем
    explicit ExportDialog(const QVector<Core::VirtualPage> *pages,
                          QWidget *parent = nullptr);

    ~ExportDialog() override;

private slots:
    void onBrowseClicked();
    void onOkClicked();

private:
    QString makeDefaultTxtPath(const QString &dir) const;
    QString makeDefaultOdtPath(const QString &dir) const;
    QString makeDefaultDocxPath(const QString &dir) const;

    Ui::ExportDialog *ui = nullptr;

    // STEP 4 → STEP 5 data (source of truth)
    const QVector<Core::VirtualPage> *m_pages = nullptr;

    QString extensionForFormat(const QString &format) const;
    QString normalizedFileName(const QString &name,
                               const QString &format) const;

};

#endif // EXPORT_H
