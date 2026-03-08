#include "Exporter.h"
#include <QPainter>

Exporter::Exporter()
{
}

QImage Exporter::render(const QImage& baseImage, const std::vector<TextBlock>& blocks)
{
    QImage result = baseImage;
    QPainter painter(&result);

    for(const auto& block : blocks)
    {
        painter.drawText(block.boundingBox, block.translatedText);
    }

    return result;
}