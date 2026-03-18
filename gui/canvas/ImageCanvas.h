#ifndef IMAGECANVAS_H
#define IMAGECANVAS_H

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QMap>
#include <vector>
#include "../../core/TextBlock.h"

class ImageCanvas : public QGraphicsView
{
    Q_OBJECT

public:
    explicit ImageCanvas(QWidget* parent = nullptr);

    void setImage(const QImage& image);
    void setBlocks(const std::vector<TextBlock>& blocks);
    void clearBlocks();
    void highlightBlock(int id);   // surligne une bulle (sync TextWindow → ImageWindow)

signals:
    void blockSelected(int id);             // clic sur une bulle
    void blockDeleteRequested(int id);      // menu contextuel → supprimer
    void addBubbleRequested(QPointF scenePos); // menu contextuel → ajouter
    void dragBubbleRequested(QRectF rect);  // drag terminé → nouvelle bulle

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QGraphicsScene*       m_scene;
    QGraphicsPixmapItem*  m_pixmapItem = nullptr;
    QMap<int, QGraphicsItem*> m_blockItems;

    // Drag pour ajout manuel
    bool    m_dragging  = false;
    QPointF m_dragStart;
    QGraphicsRectItem* m_dragRect = nullptr;

    QColor colorForConfidence(double conf) const;
    void   drawBlocks(const std::vector<TextBlock>& blocks);
};

#endif // IMAGECANVAS_H
