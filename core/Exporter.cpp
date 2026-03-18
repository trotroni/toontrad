#include "Exporter.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

// Format identique à exporter.py Python :
// ID | IMG | RAW | TRAD FR | STATUT | TRADUCTEUR | NOTE
// 001 | image.jpg | texte brut | traduction | TODO |  |

QString Exporter::exportTxt(const std::vector<Bubble>& bubbles,
                             const QString& imageName,
                             const QString& outputFolder)
{
    QDir dir(outputFolder);
    if (!dir.exists())
        dir.mkpath(".");

    QString baseName = QFileInfo(imageName).completeBaseName();
    QString filePath = dir.filePath(baseName + ".txt");

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Exporter: impossible d'ouvrir" << filePath;
        return {};
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    // Header — identique à la version Python
    out << "ID | IMG | RAW | TRAD FR | STATUT | TRADUCTEUR | NOTE\n";

    for (const auto& b : bubbles) {
        QString raw  = b.raw.simplified();
        QString trad = b.trad.simplified();

        out << QString("%1 | %2 | %3 | %4 | %5 |  | \n")
                   .arg(b.id, 3, 10, QChar('0'))
                   .arg(imageName)
                   .arg(raw)
                   .arg(trad)
                   .arg(b.status);
    }

    file.close();
    qDebug() << "Exporter: sauvegardé →" << filePath;
    return filePath;
}