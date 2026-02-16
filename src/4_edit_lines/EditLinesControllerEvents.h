// ============================================================
//  OCRtoODT — STEP 4: Preview interaction events contract
//  File: src/4_edit_lines/EditLinesControllerEvents.h
//
//  Responsibility:
//      Minimal event contract that PreviewController must expose to STEP 4.
//
//  NOTE:
//      This header is a contract reference.
//      We will NOT implement changes in PreviewController until STEP 4 implementation.
// ============================================================

#pragma once

#include <QObject>
#include <QPoint>

namespace Input {

// This is NOT a new class.
// It documents what signals STEP 4 expects PreviewController to have
// OR what adapter methods we will connect to.
class PreviewControllerEvents : public QObject
{
    Q_OBJECT
signals:
    void imageMouseMoved(const QPoint &imagePos);
    void imageMouseClicked(const QPoint &imagePos);
};

} // namespace Input
