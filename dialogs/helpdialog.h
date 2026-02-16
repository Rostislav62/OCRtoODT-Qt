// ============================================================
//  OCRtoODT — Help Dialog
//  File: helpdialog.h
//  Responsibility:
//      - Display README.md (Markdown)
//      - Provide search and navigation
//      - Work under ThemeManager styling (dark/light)
// ============================================================

#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QDialog>
#include <QTextCursor>

class SearchHighlighter;

namespace Ui {
class HelpDialog;
}

class HelpDialog : public QDialog
{
    Q_OBJECT

public:
    explicit HelpDialog(QWidget *parent = nullptr);
    ~HelpDialog();

private:
    void loadMarkdown();                     // load README.md
    void highlight(QString text);            // move to first match
    void highlightCurrentMatch(const QTextCursor &cursor);

    void updateSearchStatus();
    int m_totalMatches = 0;
    int m_currentIndex = 0;



private slots:
    void on_btnNext_clicked();
    void on_btnPrev_clicked();
    void on_editFind_textChanged(const QString &text);

private:
    Ui::HelpDialog *ui;

    QString m_content;          // whole README
    QTextCursor m_cursor;       // search cursor

    SearchHighlighter *m_highlighter = nullptr;
};

#endif // HELPDIALOG_H
