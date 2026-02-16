// ============================================================
//  OCRtoODT — Preview Controller
//  File: src/0_input/PreviewController.h
//
//  Responsibility:
//      Visual controller for page preview.
//      - Displays page image
//      - Handles zoom
//      - Emits image-space events for STEP 4
//
//  NOTE:
//      This controller is PAGE-AGNOSTIC.
//      Page ↔ Text synchronization is handled by MainWindow +
//      EditLinesController, NOT here.
//
//      No changes are required in this file for
//      Page ↔ Text Tab synchronization.
// ============================================================

#ifndef PREVIEWCONTROLLER_H
#define PREVIEWCONTROLLER_H

#include <QObject>
#include <QImage>
#include <QRect>
#include <QPoint>
#include <QEvent>

class QGraphicsView;
class QGraphicsScene;
class QGraphicsPixmapItem;
class QGraphicsRectItem;

namespace Core {
struct VirtualPage;
}

namespace Input {

class PreviewScene;

class PreviewController : public QObject
{
    Q_OBJECT
    friend class PreviewScene;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

public:
    explicit PreviewController(QGraphicsView *view, QObject *parent = nullptr);
    ~PreviewController() override;

    // Image display
    void setPreviewImage(const Core::VirtualPage &vp,
                         const QImage            &image);

    // Zoom controls
    void zoomIn();
    void zoomOut();
    void zoomFit();
    void zoom100();

    // OCR highlight (STEP 4)
    void highlightTextLine(const QRect &bbox);
    void clearTextHighlight();

signals:
    void imageClickedAt(const QPoint &imagePos);
    void imageHoveredAt(const QPoint &imagePos);

private:
    void updateFitScale();
    void applyTransform();

private:
    QGraphicsView       *m_view  = nullptr;
    QGraphicsScene      *m_scene = nullptr;
    QGraphicsPixmapItem *m_item  = nullptr;
    QGraphicsRectItem   *m_textHighlightItem = nullptr;

    QImage              m_originalImage;
    Core::VirtualPage  *m_currentPagePtr = nullptr;

    double m_currentScale = 1.0;
    double m_fitScale     = 1.0;

    const double m_zoomStep = 1.25;
    const double m_minScale = 0.10;
    const double m_maxScale = 8.00;
};

} // namespace Input

#endif // PREVIEWCONTROLLER_H
