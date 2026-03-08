#ifndef BUBBLEDETECTOR_H
#define BUBBLEDETECTOR_H

#include <QImage>
#include <vector>
#include <QRect>

class BubbleDetector
{
public:
    BubbleDetector();

    std::vector<QRect> detect(const QImage& image);
};

#endif