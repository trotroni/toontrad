#ifndef EXPORTER_H
#define EXPORTER_H

#include <QString>
#include <vector>
#include "Bubble.h"

class Exporter
{
public:
    Exporter() = default;

    // Export .txt par image — même format que exporter.py Python
    // Retourne le chemin du fichier créé, ou "" en cas d'erreur
    QString exportTxt(const std::vector<Bubble>& bubbles,
                      const QString& imageName,
                      const QString& outputFolder);
};

#endif // EXPORTER_H