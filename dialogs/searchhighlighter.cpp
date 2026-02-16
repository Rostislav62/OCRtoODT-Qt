// ============================================================
//  OCRtoODT — Search Highlighter
//  File: dialogs/searchhighlighter.cpp
//
//  Responsibility:
//      - Highlight all occurrences of a search term
//        inside a QTextDocument
//      - Maintain total match count for navigation
//
//  Notes:
//      - Case-insensitive matching
//      - No user-visible strings
//      - Does NOT participate in localization
// ============================================================

#include "searchhighlighter.h"

// ============================================================
// Constructor
// ============================================================

SearchHighlighter::SearchHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // Blue background for matches, white text
    m_format.setBackground(QColor(0x00, 0x4d, 0x94));
    m_format.setForeground(Qt::white);
}

// ============================================================
// Set search term and rehighlight document
// ============================================================

void SearchHighlighter::setSearchTerm(const QString &term)
{
    m_term = term;
    m_count = 0;
    rehighlight();
}

// ============================================================
// Clear highlighting
// ============================================================

void SearchHighlighter::clearSearch()
{
    m_term.clear();
    m_count = 0;
    rehighlight();
}

// ============================================================
// Highlight occurrences in a text block
// ============================================================

void SearchHighlighter::highlightBlock(const QString &text)
{
    if (m_term.isEmpty())
        return;

    QRegularExpression re(
        QRegularExpression::escape(m_term),
        QRegularExpression::CaseInsensitiveOption
    );

    QRegularExpressionMatchIterator it = re.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        ++m_count;

        setFormat(
            match.capturedStart(),
            match.capturedLength(),
            m_format
        );
    }
}

