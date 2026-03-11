#include "BubbleDetector.h"
#include <queue>
#include <algorithm>
#include <QDebug>

bool BubbleDetector::isWhite(QRgb pixel, int threshold) const
{
    return qRed(pixel) >= threshold &&
           qGreen(pixel) >= threshold &&
           qBlue(pixel) >= threshold;
}

QRect BubbleDetector::floodFillBounds(const QImage& img, int sx, int sy,
                                       std::vector<std::vector<bool>>& visited) const
{
    int w = img.width(), h = img.height();
    int minX = sx, maxX = sx, minY = sy, maxY = sy;

    std::queue<std::pair<int,int>> q;
    q.push({sx, sy});
    visited[sy][sx] = true;

    const int dx[] = {1,-1,0, 0};
    const int dy[] = {0, 0,1,-1};

    while (!q.empty()) {
        auto [cx, cy] = q.front(); q.pop();
        minX = std::min(minX,cx); maxX = std::max(maxX,cx);
        minY = std::min(minY,cy); maxY = std::max(maxY,cy);

        for (int d = 0; d < 4; ++d) {
            int nx = cx+dx[d], ny = cy+dy[d];
            if (nx<0||nx>=w||ny<0||ny>=h) continue;
            if (visited[ny][nx]) continue;
            if (!isWhite(img.pixel(nx,ny), 210)) continue;
            visited[ny][nx] = true;
            q.push({nx,ny});
        }
    }
    return QRect(minX, minY, maxX-minX, maxY-minY);
}

bool BubbleDetector::isValidBubble(const QRect& r, int imgW, int imgH, int minArea) const
{
    int area = r.width() * r.height();
    if (area < minArea) return false;
    if (area > imgW*imgH*8/10) return false;
    float ratio = (float)r.width() / (float)std::max(r.height(),1);
    return ratio >= 0.2f && ratio <= 5.0f;
}

std::vector<QRect> BubbleDetector::detect(const QImage& image,
                                            int whiteThreshold, int minArea)
{
    std::vector<QRect> bubbles;
    if (image.isNull()) return bubbles;

    QImage img = image.convertToFormat(QImage::Format_ARGB32);
    int w = img.width(), h = img.height();
    std::vector<std::vector<bool>> visited(h, std::vector<bool>(w, false));

    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            if (visited[y][x]) continue;
            if (!isWhite(img.pixel(x,y), whiteThreshold)) continue;
            QRect r = floodFillBounds(img, x, y, visited);
            if (isValidBubble(r, w, h, minArea))
                bubbles.push_back(r);
        }

    std::sort(bubbles.begin(), bubbles.end(),
              [](const QRect& a, const QRect& b){ return a.y() < b.y(); });

    qDebug() << "BubbleDetector (fallback):" << bubbles.size() << "bulles";
    return bubbles;
}
