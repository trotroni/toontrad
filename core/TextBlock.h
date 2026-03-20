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
    QRect    boundingBox;          // rectangle externe
    QRect    innerRect;            // rectangle interne (zone de texte)
    QPolygon polygon;
    double   confidence     = 1.0;

    QString  originalText;         // RAW
    QString  translatedText;       // TRAD FR
    QString  status         = "TODO";
    QString  notes;
    QString  translator;

    TextBlock() = default;
    TextBlock(int id, QRect box, QString text, double conf = 1.0);
    TextBlock(int id, QPolygon poly, QString text, double conf = 1.0);

    // Calcule innerRect depuis boundingBox et le ratio Config
    void computeInnerRect();

    QJsonObject toJson() const;
    static TextBlock fromJson(const QJsonObject& obj);
};

#endif // TEXTBLOCK_H
