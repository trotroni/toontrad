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

    // TXT par image (format Python original)
    bool exportTXT(const std::vector<TextBlock>& blocks,
                   const QString& imageName,
                   const QString& outputFolder);

    // JSON standard par image
    bool exportJSON(const std::vector<TextBlock>& blocks,
                    const QString& imageName,
                    const QString& filePath);

    // JSON Photoshop — coordonnées + raw + trad pour plugin PS
    bool exportPhotoshopJSON(const std::vector<TextBlock>& blocks,
                              const QString& imageName,
                              const QString& filePath);

    // PNG rendu — trad écrite sur l'image
    bool exportPNG(const QImage& base,
                   const std::vector<TextBlock>& blocks,
                   const QString& filePath);

    // Fichier consolidé toutes images (à appeler après tous les exports TXT)
    static bool exportConsolidated(const QString& outputFolder,
                                   const QString& destFile);
};

#endif // EXPORTER_H
