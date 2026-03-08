#ifndef OCRMANAGER_H
#define OCRMANAGER_H

#include <QImage>
#include <vector>
#include "TextBlock.h"

class OCRManager
{
public:
    OCRManager();

    std::vector<TextBlock> extractText(const QImage& image);
};

#endif