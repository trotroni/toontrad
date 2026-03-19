#include "ImageCanvas.h"
#include <QMenu>
#include <QAction>
#include <QAction>
#include <QScrollBar>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QDebug>

ImageCanvas::ImageCanvas(QWidget* parent)
    : QGraphicsView(parent)
    , m_scene(new QGraphicsScene(this))
{
    setScene(m_scene);
    setDragMode(QGraphicsView::NoDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setRenderHint(QPainter::Antialiasing);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setBackgroundBrush(QColor(40, 40, 40));

    // Ctrl+0 → reset zoom
    auto* resetAction = new QAction(this);
    resetAction->setShortcut(QKeySequence("Ctrl+0"));
    connect(resetAction, &QAction::triggered, this, [this]() {
        resetTransform();
        if (m_pixmapItem && !m_scene->sceneRect().isEmpty())
            fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    });
    addAction(resetAction);
}

// ─────────────────────────────────────────────────────────────────────────────
void ImageCanvas::setImage(const QImage& image)
{
    m_scene->clear();
    m_blockItems.clear();
    m_pixmapItem = nullptr;
    if (image.isNull()) return;
    m_pixmapItem = m_scene->addPixmap(QPixmap::fromImage(image));
    m_scene->setSceneRect(image.rect());
    if (!size().isEmpty())
        fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void ImageCanvas::setBlocks(const std::vector<TextBlock>& blocks)
{
    // Supprime tous les items sauf le pixmap
    for (auto* item : m_scene->items())
        if (item != m_pixmapItem)
            m_scene->removeItem(item);
    m_blockItems.clear();
    drawBlocks(blocks);
}

void ImageCanvas::clearBlocks()
{
    for (auto* item : m_scene->items())
        if (item != m_pixmapItem)
            m_scene->removeItem(item);
    m_blockItems.clear();
}

void ImageCanvas::highlightBlock(int id)
{
    // Remet tous les blocs à leur couleur normale
    for (auto it = m_blockItems.begin(); it != m_blockItems.end(); ++it) {
        if (auto* rect = dynamic_cast<QGraphicsRectItem*>(it.value())) {
            rect->setPen(QPen(QColor(0, 200, 80, 180), 2));
        }
    }
    // Surligne le bloc sélectionné
    if (m_blockItems.contains(id)) {
        if (auto* rect = dynamic_cast<QGraphicsRectItem*>(m_blockItems[id])) {
            rect->setPen(QPen(QColor(255, 220, 0), 3));
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
QColor ImageCanvas::colorForConfidence(double conf) const
{
    if (conf >= 0.8) return QColor(0, 200, 80, 180);
    if (conf >= 0.5) return QColor(255, 165, 0, 180);
    return QColor(220, 50, 50, 180);
}

void ImageCanvas::drawBlocks(const std::vector<TextBlock>& blocks)
{
    for (const auto& b : blocks) {
        QColor color = colorForConfidence(b.confidence);
        QPen   pen(color, 2);
        QBrush brush(QColor(color.red(), color.green(), color.blue(), 30));

        // Rectangle externe — vert
        auto* outer = m_scene->addRect(b.boundingBox, pen, brush);
        outer->setData(0, b.id);
        m_blockItems[b.id] = outer;

        // Rectangle interne — rouge (zone texte PS)
        if (!b.innerRect.isEmpty()) {
            QPen innerPen(QColor(255, 60, 60, 200), 1, Qt::DashLine);
            auto* inner = m_scene->addRect(b.innerRect, innerPen, QBrush());
            inner->setData(0, b.id);
        }

        // Label ID
        auto* lbl = m_scene->addText(QString::number(b.id));
        lbl->setDefaultTextColor(color);
        lbl->setPos(b.boundingBox.topLeft() + QPoint(2, 2));
        lbl->setData(0, b.id);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Zoom Ctrl+molette
// ─────────────────────────────────────────────────────────────────────────────
void ImageCanvas::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() == Qt::ControlModifier) {
        double factor = event->angleDelta().y() > 0 ? 1.25 : 1.0 / 1.25;
        scale(factor, factor);
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Souris
// ─────────────────────────────────────────────────────────────────────────────
void ImageCanvas::mousePressEvent(QMouseEvent* event)
{
    QPointF scenePos = mapToScene(event->pos());

    if (event->button() == Qt::RightButton) {
        // Menu contextuel
        QGraphicsItem* hit = m_scene->itemAt(scenePos, transform());
        int hitId = (hit && hit->data(0).isValid()) ? hit->data(0).toInt() : -1;

        QMenu menu(this);
        QAction* actAdd = menu.addAction("➕ Ajouter une bulle ici");
        QAction* actDel = nullptr;
        if (hitId > 0)
            actDel = menu.addAction(QString("🗑 Supprimer la bulle #%1").arg(hitId));

        QAction* chosen = menu.exec(event->globalPosition().toPoint());
        if (chosen == actAdd)
            emit addBubbleRequested(scenePos);
        else if (actDel && chosen == actDel)
            emit blockDeleteRequested(hitId);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        QGraphicsItem* hit = m_scene->itemAt(scenePos, transform());
        if (hit && hit->data(0).isValid()) {
            // Clic sur une bulle → sélection
            emit blockSelected(hit->data(0).toInt());
        } else {
            // Début drag pour nouvelle bulle
            m_dragging  = true;
            m_dragStart = scenePos;
            m_dragRect  = m_scene->addRect(QRectF(scenePos, scenePos),
                                            QPen(QColor(0, 120, 255), 2, Qt::DashLine),
                                            QBrush(QColor(0, 120, 255, 20)));
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void ImageCanvas::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && m_dragRect) {
        QPointF cur = mapToScene(event->pos());
        m_dragRect->setRect(QRectF(m_dragStart, cur).normalized());
    }
    QGraphicsView::mouseMoveEvent(event);
}

void ImageCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_dragging && event->button() == Qt::LeftButton) {
        m_dragging = false;
        QPointF end = mapToScene(event->pos());
        QRectF rect = QRectF(m_dragStart, end).normalized();

        if (m_dragRect) {
            m_scene->removeItem(m_dragRect);
            delete m_dragRect;
            m_dragRect = nullptr;
        }

        if (rect.width() > 5 && rect.height() > 5)
            emit dragBubbleRequested(rect);
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void ImageCanvas::resizeEvent(QResizeEvent* event)
{
    QGraphicsView::resizeEvent(event);
    if (m_pixmapItem && !m_scene->sceneRect().isEmpty())
        fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}