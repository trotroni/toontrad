#include "ImageCanvas.h"
#include <QPainter>
#include <QColor>
#include <QDebug>

ImageCanvas::ImageCanvas(QWidget* parent)
    : QLabel(parent)
{
    setAlignment(Qt::AlignTop | Qt::AlignLeft);
    setScaledContents(false);
    setMouseTracking(true);
}

// ─────────────────────────────────────────────────────────────────────────────
//  API publique
// ─────────────────────────────────────────────────────────────────────────────

void ImageCanvas::setImage(const QPixmap& pixmap)
{
    m_pixmap = pixmap;
    // Applique le zoom courant
    if (!m_pixmap.isNull()) {
        QPixmap scaled = m_pixmap.scaled(
            m_pixmap.size() * m_zoom,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
        QLabel::setPixmap(scaled);
        adjustSize();
    }
    update();
}

void ImageCanvas::setBubbles(const std::vector<Bubble>& bubbles)
{
    m_bubbles = bubbles;
    update();
}

void ImageCanvas::clearBubbles()
{
    m_bubbles.clear();
    update();
}

void ImageCanvas::setAddMode(bool enabled)
{
    m_addMode   = enabled;
    m_selecting = false;
    setCursor(enabled ? Qt::CrossCursor : Qt::ArrowCursor);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Conversions coordonnées
// ─────────────────────────────────────────────────────────────────────────────

QPoint ImageCanvas::toImage(const QPoint& widgetPos) const
{
    return QPoint(
        static_cast<int>(widgetPos.x() / m_zoom),
        static_cast<int>(widgetPos.y() / m_zoom)
    );
}

QPoint ImageCanvas::toWidget(const QPoint& imagePos) const
{
    return QPoint(
        static_cast<int>(imagePos.x() * m_zoom),
        static_cast<int>(imagePos.y() * m_zoom)
    );
}

// ─────────────────────────────────────────────────────────────────────────────
//  Zoom — Ctrl + molette (identique au Python)
// ─────────────────────────────────────────────────────────────────────────────

void ImageCanvas::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() == Qt::ControlModifier) {
        double factor = event->angleDelta().y() > 0 ? 1.1 : (1.0 / 1.1);
        m_zoom = qBound(0.1, m_zoom * factor, 10.0);
        setImage(m_pixmap); // redessine avec nouveau zoom
        event->accept();
    } else {
        event->ignore(); // laisse le QScrollArea gérer le scroll normal
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Souris
// ─────────────────────────────────────────────────────────────────────────────

void ImageCanvas::mousePressEvent(QMouseEvent* event)
{
    QPoint imgPos = toImage(event->pos());

    // ── Clic droit : suppression bulle ───────────────────────────────────────
    if (event->button() == Qt::RightButton) {
        for (const auto& b : m_bubbles) {
            if (b.rect.contains(imgPos)) {
                emit bubbleDeleteRequested(b.id);
                return;
            }
        }
    }

    // ── Clic gauche en mode ajout : début du drag ────────────────────────────
    if (m_addMode && event->button() == Qt::LeftButton) {
        m_selecting   = true;
        m_dragStart   = imgPos;
        m_dragCurrent = imgPos;
    }
}

void ImageCanvas::mouseMoveEvent(QMouseEvent* event)
{
    if (m_selecting) {
        m_dragCurrent = toImage(event->pos());
        update(); // redessine le rectangle de sélection en cours
    }
}

void ImageCanvas::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_selecting && event->button() == Qt::LeftButton) {
        m_selecting = false;

        int x1 = qMin(m_dragStart.x(), m_dragCurrent.x());
        int y1 = qMin(m_dragStart.y(), m_dragCurrent.y());
        int x2 = qMax(m_dragStart.x(), m_dragCurrent.x());
        int y2 = qMax(m_dragStart.y(), m_dragCurrent.y());
        int w  = x2 - x1;
        int h  = y2 - y1;

        if (w > 5 && h > 5) // ignore les micro-clics
            emit bubbleAddRequested(QRect(x1, y1, w, h));

        setAddMode(false);
        update();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Dessin — rectangles bulles (identique au Python : vert outer, rouge inner)
// ─────────────────────────────────────────────────────────────────────────────

void ImageCanvas::paintEvent(QPaintEvent* event)
{
    // Dessine d'abord l'image (QLabel)
    QLabel::paintEvent(event);

    if (m_pixmap.isNull()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // ── Bulles existantes ────────────────────────────────────────────────────
    for (const auto& b : m_bubbles) {
        // Rectangle externe — vert (comme Python)
        QRect outerW = QRect(toWidget(b.rect.topLeft()),
                             toWidget(b.rect.bottomRight()));
        painter.setPen(QPen(QColor(0, 255, 0), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(outerW);

        // Rectangle interne — rouge (comme Python)
        QRect innerW = QRect(toWidget(b.innerRect.topLeft()),
                             toWidget(b.innerRect.bottomRight()));
        painter.setPen(QPen(QColor(255, 0, 0), 1));
        painter.drawRect(innerW);

        // Label ID : texte au-dessus du rect (comme Python)
        QString label = QString("%1: %2")
                            .arg(b.id)
                            .arg(b.trad.isEmpty()
                                     ? b.raw.left(30)
                                     : b.trad.left(30));
        painter.setPen(QColor(0, 255, 0));
        painter.drawText(outerW.topLeft() + QPoint(0, -4), label);
    }

    // ── Rectangle de sélection en cours (mode ajout) ─────────────────────────
    if (m_selecting) {
        QRect selW = QRect(toWidget(m_dragStart),
                           toWidget(m_dragCurrent)).normalized();
        painter.setPen(QPen(QColor(0, 0, 255, 180), 2, Qt::DashLine));
        painter.setBrush(QColor(0, 0, 255, 30));
        painter.drawRect(selW);
    }
}