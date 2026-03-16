#ifndef IMAGECANVAS_H
#define IMAGECANVAS_H

#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsPolygonItem>
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
    void removeBlock(int id);

signals:
    void blockRightClicked(int id);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QGraphicsScene* m_scene;
    QGraphicsPixmapItem*    m_pixmapItem = nullptr;
    QMap<int, QGraphicsItem*> m_items;
    QColor colorForConfidence(double conf) const;
};

#endif // IMAGECANVAS_H
