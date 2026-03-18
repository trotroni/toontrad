#include "Bubble.h"
#include "../config.h"
#include <QJsonArray>

// Même logique que compute_inner_rect() Python :
//   new_w = w * ratio
//   new_h = h * ratio
//   offset_x = (w - new_w) / 2
//   offset_y = (h - new_h) / 2
QRect Bubble::computeInnerRect(const QRect& r)
{
    double ratio = Config::innerRectRatio;

    int nw = static_cast<int>(r.width()  * ratio);
    int nh = static_cast<int>(r.height() * ratio);
    int ox = (r.width()  - nw) / 2;
    int oy = (r.height() - nh) / 2;

    return QRect(r.x() + ox, r.y() + oy, nw, nh);
}

QJsonObject Bubble::toJson() const
{
    QJsonObject obj;
    obj["id"]     = id;
    obj["raw"]    = raw;
    obj["trad"]   = trad;
    obj["status"] = status;

    // rect : [x, y, w, h]
    QJsonArray r;
    r << rect.x() << rect.y() << rect.width() << rect.height();
    obj["rect"] = r;

    // inner_rect
    QJsonArray ir;
    ir << innerRect.x() << innerRect.y() << innerRect.width() << innerRect.height();
    obj["inner_rect"] = ir;

    return obj;
}

Bubble Bubble::fromJson(const QJsonObject& obj)
{
    Bubble b;
    b.id     = obj["id"].toInt();
    b.raw    = obj["raw"].toString();
    b.trad   = obj["trad"].toString();
    b.status = obj["status"].toString("TODO");

    QJsonArray r = obj["rect"].toArray();
    if (r.size() == 4)
        b.rect = QRect(r[0].toInt(), r[1].toInt(), r[2].toInt(), r[3].toInt());

    // inner_rect : recalculé si absent (rétrocompatibilité)
    if (obj.contains("inner_rect")) {
        QJsonArray ir = obj["inner_rect"].toArray();
        if (ir.size() == 4)
            b.innerRect = QRect(ir[0].toInt(), ir[1].toInt(),
                                ir[2].toInt(), ir[3].toInt());
    } else {
        b.innerRect = computeInnerRect(b.rect);
    }

    return b;
}