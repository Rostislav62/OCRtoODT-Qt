// ============================================================
//  OCRtoODT — Clear Controller
//  File: src/0_input/ClearController.h
//
//  Responsibility:
//      UI-level controller responsible for clearing current input state.
//
//      This controller coordinates RESET operations for:
//
//          - InputController        (STEP 0_input data)
//          - PreviewController      (preview image)
//          - File list model (UI)
//
//      IMPORTANT:
//          • ClearController does NOT perform any logic itself
//          • It only emits requests to other controllers
//          • It does NOT know how reset is implemented internally
//
//      DESIGN GOAL:
//          Keep MainWindow free of reset/cleanup logic.
// ============================================================

#ifndef CLEARCONTROLLER_H
#define CLEARCONTROLLER_H

#include <QObject>

namespace Input {

class ClearController : public QObject
{
    Q_OBJECT

public:
    explicit ClearController(QObject *parent = nullptr);

    // Called by UI (menu / toolbar)
    void clearAll();

    // UI state helper
    void setHasFiles(bool hasFiles);

signals:
    // Requests sent to other controllers
    void requestControllerReset();   // InputController
    void requestModelClear();        // File list
    void requestPreviewClear();      // PreviewController

    // UI enable/disable notifications
    void uiStateChanged(bool hasFiles);

private:
    bool m_hasFiles = false;
};

} // namespace Input

#endif // CLEARCONTROLLER_H
