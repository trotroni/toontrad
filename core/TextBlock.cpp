#include "TextBlock.h"

TextBlock::TextBlock()
{
}

TextBlock::TextBlock(QRect box, QString text)
{
    boundingBox = box;
    originalText = text;
}