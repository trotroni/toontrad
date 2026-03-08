#ifndef EXPORTER_H
#define EXPORTER_H

#include <QImage>
#include <vector>
#include "TextBlock.h"

class Exporter
{
public:
    Exporter();

    QImage render(const QImage& baseImage, const std::vector<TextBlock>& blocks);
};

#endif