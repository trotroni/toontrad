#include "OCRManager.h"
#include <QImage>
#include <QDebug>

OCRManager::OCRManager()
{
}

std::vector<TextBlock> OCRManager::extractText(const QImage& image)
{
    std::vector<TextBlock> blocks;

    if(image.isNull())
    {
        qDebug() << "Image non chargée";
        return blocks;
    }

    int w = image.width();
    int h = image.height();

    // Simulation de 3 bulles OCR
    QRect box1(w * 0.1, h * 0.1, w * 0.3, h * 0.15);
    QRect box2(w * 0.5, h * 0.2, w * 0.35, h * 0.15);
    QRect box3(w * 0.2, h * 0.5, w * 0.4, h * 0.2);

    blocks.emplace_back(box1, "Hello there!");
    blocks.emplace_back(box2, "This is a test bubble.");
    blocks.emplace_back(box3, "ToonTrad OCR simulation.");

    qDebug() << "OCR simulation : " << blocks.size() << " blocs détectés";

    return blocks;
}