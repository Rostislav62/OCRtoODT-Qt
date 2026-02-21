// ============================================================
//  OCRtoODT — Help Dialog
//  File: helpdialog.cpp
//  Responsibility:
//      - Load README.md from resources
//      - Show Markdown in QTextBrowser
//      - Provide search & navigation with highlighting
//      - Styling is applied via ThemeManager
// ============================================================

#include "helpdialog.h"
#include "ui_helpdialog.h"

#include "searchhighlighter.h"        // our search highlighter

#include <QFile>
#include <QTextStream>
#include <QTextDocument>
#include <QDialogButtonBox>
#include <QShortcut>
#include <QKeySequence>
#include <QIcon>
#include <QPixmap>

// ============================================================
//  Constructor
// ============================================================
HelpDialog::HelpDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::HelpDialog)
{
    // Load UI
    ui->setupUi(this);

    // --------------------------------------------------------
    // Set help icon in header
    // --------------------------------------------------------
    QIcon helpIcon(":/icons/icons/help.svg");
    QPixmap pix = helpIcon.pixmap(48, 48);
    ui->lblLogo->setPixmap(pix);
    ui->lblLogo->setScaledContents(true);

    // --------------------------------------------------------
    // Window sizing
    // --------------------------------------------------------
    this->resize(900, 700);
    this->setMinimumSize(600, 400);

    // --------------------------------------------------------
    // Safe Linux-friendly font
    // --------------------------------------------------------
    QFont f("DejaVu Sans Mono", 11);
    f.setStyleStrategy(QFont::PreferAntialias);
    ui->textBrowser->setFont(f);

    // --------------------------------------------------------
    // Close button → close dialog
    // --------------------------------------------------------
    connect(ui->buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);

    // --------------------------------------------------------
    // Ctrl+F → focus the search field
    // --------------------------------------------------------
    QShortcut *shortcutFind = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(shortcutFind, &QShortcut::activated, this, [this]() {
        ui->editFind->setFocus();
        ui->editFind->selectAll();
    });

    // ------------------------------------------------------------
    // Enter → Next match
    // Shift+Enter → Previous match
    // ------------------------------------------------------------
    QShortcut *shortcutEnter = new QShortcut(QKeySequence(Qt::Key_Return), this);
    connect(shortcutEnter, &QShortcut::activated, this, [this]() {
        on_btnNext_clicked();
    });

    QShortcut *shortcutShiftEnter =
        new QShortcut(QKeySequence(Qt::SHIFT | Qt::Key_Return), this);
    connect(shortcutShiftEnter, &QShortcut::activated, this, [this]() {
        on_btnPrev_clicked();
    });

    // --------------------------------------------------------
    // Load documentation
    // --------------------------------------------------------
    loadMarkdown();

    // --------------------------------------------------------
    // Create highlighter
    // --------------------------------------------------------
    m_highlighter = new SearchHighlighter(ui->textBrowser->document());

    // Prepare cursor
    m_cursor = ui->textBrowser->textCursor();
}

// ============================================================
// Destructor
// ============================================================
HelpDialog::~HelpDialog()
{
    delete ui;
}

// ============================================================
// Load README.md → QTextBrowser (Markdown)
// ============================================================
void HelpDialog::loadMarkdown()
{
    qDebug() << "Resource exists?"
             << QFile::exists(":/docs/README.md");

    QFile file(":/docs/README.md");

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ui->textBrowser->setText("ERROR: Could not load README.md");
        return;

    }

    m_content = QString::fromUtf8(file.readAll());
    file.close();

    ui->textBrowser->document()->setBaseUrl(QUrl("qrc:/docs/"));
    ui->textBrowser->setMarkdown(m_content);

}

// ============================================================
// Highlight current match
// ============================================================
void HelpDialog::highlightCurrentMatch(const QTextCursor &cursor)
{
    if (cursor.isNull()) {
        ui->textBrowser->setExtraSelections({});
        return;
    }

    QTextEdit::ExtraSelection sel;
    sel.cursor = cursor;
    sel.format.setBackground(QColor(0x00, 0x78, 0xD4));
    sel.format.setForeground(Qt::white);

    ui->textBrowser->setExtraSelections({ sel });
}

// ============================================================
//  Update "3 / 12" status text
// ============================================================
void HelpDialog::updateSearchStatus()
{
    if (m_totalMatches == 0) {
        ui->lblStatus->setText("0 / 0");
        return;
    }

    ui->lblStatus->setText(
        QString("%1 / %2")
            .arg(m_currentIndex)
            .arg(m_totalMatches)
        );
}

// ============================================================
// Move to first match
// ============================================================
void HelpDialog::highlight(QString text)
{
    QTextDocument *doc = ui->textBrowser->document();

    if (text.isEmpty()) {
        m_cursor = QTextCursor(doc);
        ui->textBrowser->setExtraSelections({});
        ui->textBrowser->moveCursor(QTextCursor::Start);
        return;
    }

    m_cursor = doc->find(text);

    if (!m_cursor.isNull()) {
        ui->textBrowser->setTextCursor(m_cursor);
        ui->textBrowser->ensureCursorVisible();
        highlightCurrentMatch(m_cursor);
    }
}

// ============================================================
// NEXT (wrap around)
// ============================================================
void HelpDialog::on_btnNext_clicked()
{
    QString text = ui->editFind->text();
    if (text.isEmpty() || m_totalMatches == 0)
        return;

    QTextDocument *doc = ui->textBrowser->document();

    m_cursor = doc->find(text, m_cursor, QTextDocument::FindFlags());

    if (m_cursor.isNull()) {
        QTextCursor start(doc);
        m_cursor = doc->find(text, start, QTextDocument::FindFlags());
    }

    if (!m_cursor.isNull()) {
        ui->textBrowser->setTextCursor(m_cursor);
        ui->textBrowser->ensureCursorVisible();
        highlightCurrentMatch(m_cursor);

        m_currentIndex = (m_currentIndex % m_totalMatches) + 1;
        updateSearchStatus();
    }
}

// ============================================================
// PREVIOUS (wrap around)
// ============================================================
void HelpDialog::on_btnPrev_clicked()
{
    QString text = ui->editFind->text();
    if (text.isEmpty() || m_totalMatches == 0)
        return;

    QTextDocument *doc = ui->textBrowser->document();

    m_cursor = doc->find(text, m_cursor, QTextDocument::FindBackward);

    if (m_cursor.isNull()) {
        QTextCursor end(doc);
        end.movePosition(QTextCursor::End);
        m_cursor = doc->find(text, end, QTextDocument::FindBackward);
    }

    if (!m_cursor.isNull()) {
        ui->textBrowser->setTextCursor(m_cursor);
        ui->textBrowser->ensureCursorVisible();
        highlightCurrentMatch(m_cursor);

        if (m_currentIndex <= 1)
            m_currentIndex = m_totalMatches;
        else
            --m_currentIndex;

        updateSearchStatus();
    }
}

// ============================================================
//  Live search when typing
// ============================================================
void HelpDialog::on_editFind_textChanged(const QString &text)
{
    if (text.isEmpty()) {
        m_highlighter->clearSearch();
        m_totalMatches = 0;
        m_currentIndex = 0;
        updateSearchStatus();
        ui->textBrowser->moveCursor(QTextCursor::Start);
        return;
    }

    m_highlighter->setSearchTerm(text);
    m_totalMatches = m_highlighter->matchCount();

    highlight(text);

    m_currentIndex = (m_totalMatches > 0 ? 1 : 0);
    updateSearchStatus();
}
