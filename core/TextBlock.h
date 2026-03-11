#ifndef TEXTBLOCK_H
#define TEXTBLOCK_H

#include <QString>
#include <QRect>
#include <QPolygon>
#include <QJsonObject>
#include <QJsonArray>

class TextBlock
{
public:
    int      id             = 0;
    QRect    boundingBox;
    QPolygon polygon;
    double   confidence     = 1.0;
    QString  originalText;
    QString  translatedText;

    TextBlock() = default;
    TextBlock(int id, QRect box, QString text, double conf = 1.0);
    TextBlock(int id, QPolygon poly, QString text, double conf = 1.0);

    QJsonObject toJson() const;
    static TextBlock fromJson(const QJsonObject& obj);
};

#endif // TEXTBLOCK_H
