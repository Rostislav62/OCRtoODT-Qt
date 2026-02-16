// ============================================================
//  OCRtoODT — Search Highlighter
//  File: searchhighlighter.h
//  Responsibility:
//      - Highlight all occurrences of a search term
//      - Count total matches for status "3 / 12"
// ============================================================

#ifndef SEARCHHIGHLIGHTER_H
#define SEARCHHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

class SearchHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit SearchHighlighter(QTextDocument *parent = nullptr);

    // Set search term and re-highlight document
    void setSearchTerm(const QString &term);

    // Clear highlighting
    void clearSearch();

    // Total matches in whole document
    int matchCount() const { return m_count; }

protected:
    // Called by QSyntaxHighlighter for each block (line)
    void highlightBlock(const QString &text) override;

private:
    QString         m_term;     // current search term
    QTextCharFormat m_format;   // highlight style
    int             m_count = 0; // number of matches in whole document
};

#endif // SEARCHHIGHLIGHTER_H
