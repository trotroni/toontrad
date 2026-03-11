#ifndef EXPORTER_H
#define EXPORTER_H

#include <QImage>
#include <QString>
#include <vector>
#include "TextBlock.h"

class Exporter
{
public:
    Exporter() = default;

    QImage renderImage(const QImage& base, const std::vector<TextBlock>& blocks);

    bool exportJSON(const std::vector<TextBlock>& blocks,
                    const QString& imageName,
                    const QString& filePath);

    bool exportTXT(const std::vector<TextBlock>& blocks,
                   const QString& imageName,
                   const QString& filePath);
};

#endif // EXPORTER_H
