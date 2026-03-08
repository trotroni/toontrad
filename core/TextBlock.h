#ifndef TEXTBLOCK_H
#define TEXTBLOCK_H

#include <QString>
#include <QRect>

class TextBlock
{
public:
    QRect boundingBox;
    QString originalText;
    QString translatedText;

    TextBlock();
    TextBlock(QRect box, QString text);
};

#endif