// ============================================================
//  OCRtoODT — Settings: Logging Pane
//  File: settings/loggingpane.h
//
//  STEP 6.1.4
//  Synchronization: Settings UI → ConfigManager → LogRouter
//
// ============================================================

#ifndef LOGGINGPANE_H
#define LOGGINGPANE_H

#include <QWidget>

namespace Ui {
class LoggingPane;
}

class LoggingPane : public QWidget
{
    Q_OBJECT

public:
    explicit LoggingPane(QWidget *parent = nullptr);
    ~LoggingPane();

    // Load values from config.yaml → UI
    void loadFromConfig();

    // Apply UI → ConfigManager + live LogRouter update
    void applyToConfig();

private slots:
    void onBrowse();

private:
    Ui::LoggingPane *ui = nullptr;

    int  detectSelectedLogLevel() const;
    void selectLogLevel(int level);
};

#endif // LOGGINGPANE_H
