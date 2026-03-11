#ifndef BUBBLEDETECTOR_H
#define BUBBLEDETECTOR_H

#include <QImage>
#include <QRect>
#include <vector>

class BubbleDetector
{
public:
    BubbleDetector() = default;

    std::vector<QRect> detect(const QImage& image,
                              int whiteThreshold = 210,
                              int minArea        = 2000);

private:
    bool  isWhite(QRgb pixel, int threshold) const;
    QRect floodFillBounds(const QImage& img, int x, int y,
                          std::vector<std::vector<bool>>& visited) const;
    bool  isValidBubble(const QRect& r, int imgW, int imgH, int minArea) const;
};

#endif // BUBBLEDETECTOR_H
