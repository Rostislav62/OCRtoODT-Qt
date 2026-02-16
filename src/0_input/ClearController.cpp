// ============================================================
//  OCRtoODT — Clear Controller
//  File: src/0_input/ClearController.cpp
//
//  Responsibility:
//      Emit coordinated reset requests for STEP 0_input UI state.
//
//      This controller:
//          - does NOT know implementation details
//          - does NOT manipulate models directly
//          - only emits clear/reset signals
// ============================================================

#include "ClearController.h"

#include "core/LogRouter.h"

namespace Input {

// ------------------------------------------------------------
// Constructor
// ------------------------------------------------------------
ClearController::ClearController(QObject *parent)
    : QObject(parent)
{
}

// ------------------------------------------------------------
// clearAll
//  Invoked by MainWindow when user clicks "Clear"
// ------------------------------------------------------------
void ClearController::clearAll()
{
    if (!m_hasFiles)
        return;

    ::LogRouter::instance().info("[ClearController] Clearing input state");

    emit requestControllerReset();   // InputController::reset()
    emit requestModelClear();        // UI file list
    emit requestPreviewClear();      // Preview image

    m_hasFiles = false;
    emit uiStateChanged(false);
}

// ------------------------------------------------------------
// setHasFiles
//  Called by MainWindow after files are loaded or cleared
// ------------------------------------------------------------
void ClearController::setHasFiles(bool hasFiles)
{
    m_hasFiles = hasFiles;
    emit uiStateChanged(hasFiles);
}

} // namespace Input
