#ifndef IMAGECANVAS_H
#define IMAGECANVAS_H

#include <QLabel>
#include <QPixmap>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>
#include <vector>
#include "../core/Bubble.h"

class ImageCanvas : public QLabel
{
    Q_OBJECT

public:
    explicit ImageCanvas(QWidget* parent = nullptr);

    void setImage(const QPixmap& pixmap);
    void setBubbles(const std::vector<Bubble>& bubbles);
    void clearBubbles();

    double zoom() const { return m_zoom; }

    signals:
        // Clic droit sur une bulle → suppression
        void bubbleDeleteRequested(int id);

    // Drag terminé en mode ajout → nouveau rect sélectionné
    void bubbleAddRequested(QRect rect);

public slots:
    void setAddMode(bool enabled);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    double              m_zoom      = 1.0;
    bool                m_addMode   = false;

    // Drag sélection
    bool                m_selecting = false;
    QPoint              m_dragStart;
    QPoint              m_dragCurrent;

    QPixmap             m_pixmap;
    std::vector<Bubble> m_bubbles;

    // Convertit coordonnées widget → coordonnées image
    QPoint toImage(const QPoint& widgetPos) const;
    QPoint toWidget(const QPoint& imagePos) const;
};

#endif // IMAGECANVAS_H