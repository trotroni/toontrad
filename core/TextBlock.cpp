#include "TextBlock.h"
#include <QJsonArray>

TextBlock::TextBlock(int id, QRect box, QString text, double conf)
    : id(id), boundingBox(box), confidence(conf), originalText(std::move(text))
{
    polygon << box.topLeft() << box.topRight()
            << box.bottomRight() << box.bottomLeft();
}

TextBlock::TextBlock(int id, QPolygon poly, QString text, double conf)
    : id(id), polygon(poly), confidence(conf), originalText(std::move(text))
{
    boundingBox = poly.boundingRect();
}

QJsonObject TextBlock::toJson() const
{
    QJsonObject obj;
    obj["id"]             = id;
    obj["confidence"]     = confidence;
    obj["originalText"]   = originalText;
    obj["translatedText"] = translatedText;

    QJsonObject box;
    box["x"] = boundingBox.x();
    box["y"] = boundingBox.y();
    box["w"] = boundingBox.width();
    box["h"] = boundingBox.height();
    obj["boundingBox"] = box;

    QJsonArray pts;
    for (const QPoint& p : polygon) {
        QJsonArray pt;
        pt.append(p.x());
        pt.append(p.y());
        pts.append(pt);
    }
    obj["polygon"] = pts;

    return obj;
}

TextBlock TextBlock::fromJson(const QJsonObject& obj)
{
    TextBlock b;
    b.id             = obj["id"].toInt();
    b.confidence     = obj["confidence"].toDouble(1.0);
    b.originalText   = obj["originalText"].toString();
    b.translatedText = obj["translatedText"].toString();

    if (obj.contains("boundingBox")) {
        QJsonObject box = obj["boundingBox"].toObject();
        b.boundingBox = QRect(box["x"].toInt(), box["y"].toInt(),
                              box["w"].toInt(), box["h"].toInt());
    }

    if (obj.contains("polygon")) {
        QPolygon poly;
        for (const QJsonValue& v : obj["polygon"].toArray()) {
            QJsonArray pt = v.toArray();
            if (pt.size() >= 2)
                poly << QPoint(pt[0].toInt(), pt[1].toInt());
        }
        b.polygon = poly;
    } else {
        b.polygon << b.boundingBox.topLeft()     << b.boundingBox.topRight()
                  << b.boundingBox.bottomRight() << b.boundingBox.bottomLeft();
    }

    return b;
}
