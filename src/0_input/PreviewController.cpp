// ============================================================
//  OCRtoODT — Preview Controller
//  File: src/0_input/PreviewController.cpp
//
//  Responsibility:
//      Display a single page image inside QGraphicsView and translate
//      user mouse activity into IMAGE coordinates.
//
//      Emits:
//          • imageHoveredAt(imagePos)
//          • imageClickedAt(imagePos)
//
//      NOTE:
//          This file does NOT participate in Page ↔ Text
//          synchronization logic and therefore requires NO changes.
// ============================================================

#include "0_input/PreviewController.h"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QMouseEvent>
#include <QPainter>
#include <QTransform>

#include "core/VirtualPage.h"
#include "core/LogRouter.h"

namespace Input {

bool m_autoFitEnabled = true;

PreviewController::PreviewController(QGraphicsView *view, QObject *parent)
    : QObject(parent)
    , m_view(view)
{


    m_scene = new QGraphicsScene(this);

    if (m_view)
    {
        m_view->setScene(m_scene);
        m_view->setRenderHint(QPainter::Antialiasing, true);
        m_view->setRenderHint(QPainter::SmoothPixmapTransform, true);
        m_view->setMouseTracking(true);

        if (m_view)
        {
            m_view->setMouseTracking(true);
            m_view->viewport()->installEventFilter(this);
        }

    }

    LogRouter::instance().info(
        "[PreviewController] Initialized");
}

PreviewController::~PreviewController() = default;

bool PreviewController::eventFilter(QObject *obj, QEvent *event)
{
    if (!m_view || !m_view->viewport())
        return QObject::eventFilter(obj, event);

    if (obj != m_view->viewport())
        return QObject::eventFilter(obj, event);

    // --------------------------------------------------------
    // Resize-aware auto-fit
    // --------------------------------------------------------
    if (event->type() == QEvent::Resize)
    {
        if (m_autoFitEnabled && m_item && !m_originalImage.isNull())
        {
            QMetaObject::invokeMethod(
                this,
                [this]()
                {
                    zoomFit();
                },
                Qt::QueuedConnection);
        }
    }



    // --------------------------------------------------------
    // Mouse handling
    // --------------------------------------------------------
    if (!m_item || m_originalImage.isNull())
        return QObject::eventFilter(obj, event);

    if (event->type() == QEvent::MouseMove ||
        event->type() == QEvent::MouseButtonPress)
    {
        auto *me = static_cast<QMouseEvent*>(event);
        const QPointF scenePos = m_view->mapToScene(me->pos());
        const QPointF itemPos  = m_item->mapFromScene(scenePos);
        const QPoint imagePos(int(itemPos.x()), int(itemPos.y()));

        if (imagePos.x() >= 0 && imagePos.y() >= 0 &&
            imagePos.x() < m_originalImage.width() &&
            imagePos.y() < m_originalImage.height())
        {
            if (event->type() == QEvent::MouseMove)
                emit imageHoveredAt(imagePos);
            else
                emit imageClickedAt(imagePos);
        }
    }

    return QObject::eventFilter(obj, event);
}


void PreviewController::setPreviewImage(const Core::VirtualPage &vp,
                                        const QImage            &image)
{
    m_currentPagePtr = const_cast<Core::VirtualPage*>(&vp);

    m_scene->clear();
    m_item = nullptr;
    m_textHighlightItem = nullptr;

    if (image.isNull())
        return;

    m_originalImage = image;
    m_item = m_scene->addPixmap(QPixmap::fromImage(image));

    updateFitScale();
    m_currentScale = m_fitScale;
    applyTransform();
}

void PreviewController::zoomIn()
{
    m_autoFitEnabled = false;
    m_currentScale = std::min(m_currentScale * m_zoomStep, m_maxScale);
    applyTransform();
}

void PreviewController::zoomOut()
{
    m_autoFitEnabled = false;
    m_currentScale = std::max(m_currentScale / m_zoomStep, m_minScale);
    applyTransform();
}

void PreviewController::zoomFit()
{
    m_autoFitEnabled = true;
    updateFitScale();
    m_currentScale = m_fitScale;
    applyTransform();
}

void PreviewController::zoom100()
{
    m_autoFitEnabled = false;
    m_currentScale = 1.0;
    applyTransform();
}

void PreviewController::updateFitScale()
{
    if (!m_view || m_originalImage.isNull())
        return;

    const QSize vs = m_view->viewport()->size();
    if (vs.isEmpty())
        return;

    const double sx = double(vs.width())  / m_originalImage.width();
    const double sy = double(vs.height()) / m_originalImage.height();
    m_fitScale = std::min(sx, sy);
}

void PreviewController::applyTransform()
{
    if (!m_item)
        return;

    QTransform t;
    t.scale(m_currentScale, m_currentScale);
    m_view->setTransform(t);
    m_view->centerOn(m_item);
}

void PreviewController::highlightTextLine(const QRect &bbox)
{
    if (!m_textHighlightItem)
    {
        m_textHighlightItem = new QGraphicsRectItem();
        m_textHighlightItem->setPen(QPen(Qt::red, 2));
        m_textHighlightItem->setBrush(Qt::NoBrush);
        m_scene->addItem(m_textHighlightItem);
    }

    m_textHighlightItem->setRect(bbox);
    m_textHighlightItem->setVisible(true);
}

void PreviewController::clearTextHighlight()
{
    if (m_textHighlightItem)
        m_textHighlightItem->setVisible(false);
}

} // namespace Input
