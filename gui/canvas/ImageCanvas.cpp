#include "ImageCanvas.h"
#include <QScrollBar>
#include <QMenu>
#include <QAction>
#include <QDebug>

ImageCanvas::ImageCanvas(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
{
    setScene(m_scene);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setRenderHint(QPainter::Antialiasing);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setBackgroundBrush(QColor(40, 40, 40));
}

void ImageCanvas::setImage(const QImage& image)
{
    m_scene->clear();
    m_items.clear();
    if (image.isNull()) return;

    m_scene->addPixmap(QPixmap::fromImage(image));
    m_scene->setSceneRect(image.rect());
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

QColor ImageCanvas::colorForConfidence(double conf) const
{
    if (conf >= 0.8) return QColor(0, 200, 80, 180);
    if (conf >= 0.5) return QColor(255, 165, 0, 180);
    return QColor(220, 50, 50, 180);
}

void ImageCanvas::setBlocks(const std::vector<TextBlock>& blocks)
{
    QList<QGraphicsItem*> all = m_scene->items();
    for (auto* it : all)
        if (dynamic_cast<QGraphicsPixmapItem*>(it) == nullptr)
            m_scene->removeItem(it);
    m_items.clear();

    for (const auto& b : blocks) {
        QColor color = colorForConfidence(b.confidence);
        QPen   pen(color, 2);
        QBrush brush(QColor(color.red(), color.green(), color.blue(), 40));

        QGraphicsItem* item = nullptr;
        if (b.polygon.size() >= 3) {
            auto* poly = m_scene->addPolygon(QPolygonF(b.polygon), pen, brush);
            poly->setData(0, b.id);
            item = poly;
        } else {
            auto* rect = m_scene->addRect(b.boundingBox, pen, brush);
            rect->setData(0, b.id);
            item = rect;
        }

        QGraphicsTextItem* lbl = m_scene->addText(QString::number(b.id));
        lbl->setDefaultTextColor(color);
        lbl->setPos(b.boundingBox.topLeft() + QPoint(2, 2));
        lbl->setData(0, b.id);

        m_items[b.id] = item;
    }
}

void ImageCanvas::clearBlocks()
{
    QList<QGraphicsItem*> all = m_scene->items();
    for (auto* it : all)
        if (!dynamic_cast<QGraphicsPixmapItem*>(it))
            m_scene->removeItem(it);
    m_items.clear();
}

void ImageCanvas::removeBlock(int id)
{
    QList<QGraphicsItem*> all = m_scene->items();
    for (auto* it : all) {
        if (it->data(0).toInt() == id)
            m_scene->removeItem(it);
    }
    m_items.remove(id);
}

void ImageCanvas::wheelEvent(QWheelEvent* event)
{
    double factor = event->angleDelta().y() > 0 ? 1.15 : 1.0/1.15;
    scale(factor, factor);
}

void ImageCanvas::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::RightButton) {
        QPointF scenePos = mapToScene(event->pos());
        QGraphicsItem* hit = m_scene->itemAt(scenePos, transform());
        if (hit && hit->data(0).isValid()) {
            int id = hit->data(0).toInt();
            QMenu menu(this);
            QAction* del = menu.addAction(
                QString("Supprimer la zone #%1").arg(id));
            if (menu.exec(event->globalPosition().toPoint()) == del)
                emit blockRightClicked(id);
            return;
        }
    }
    QGraphicsView::mousePressEvent(event);
}
