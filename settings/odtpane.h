#ifndef ODTPANE_H
#define ODTPANE_H

#include <QWidget>
#include <QRect>

namespace Ui {
class ODTSettingsPane;
}

class ODTSettingsPane : public QWidget
{
    Q_OBJECT

public:
    explicit ODTSettingsPane(QWidget *parent = nullptr);
    ~ODTSettingsPane();

    void load();
    void save();

    // ------------------------------------------------------------
    // Preview text formatting (used by ODTSettingsPane)
    // ------------------------------------------------------------
    void setPreviewFont(const QFont &font);
    void setPreviewAlignment(Qt::Alignment align);


private slots:
    // ========================================================
    // Localization support
    // ========================================================
    void retranslate();


private:
    Ui::ODTSettingsPane *ui;

    void loadODT();
    void saveODT();
    void updatePagePreview();

    void applyAlignmentFromConfig(const QString &align);
    QString alignmentToString() const;

    // ------------------------------------------------------------
    // Preview text state
    // ------------------------------------------------------------
    QFont m_previewFont;
    Qt::Alignment m_previewAlignment = Qt::AlignJustify;

    // Sample preview text
    QString m_previewTitle;
    QString m_previewText;

    // --------------------------------------------------------
    // Helpers
    // --------------------------------------------------------
    Qt::Alignment parseAlignmentFromString(const QString &align) const;

};

#endif // ODTPANE_H

