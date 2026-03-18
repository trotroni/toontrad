#include "Exporter.h"
#include <QPainter>
#include <QFont>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextStream>
#include <QDirIterator>
#include <QDebug>

// ─── TXT par image ────────────────────────────────────────────────────────────
bool Exporter::exportTXT(const std::vector<TextBlock>& blocks,
                          const QString& imageName,
                          const QString& outputFolder)
{
    QDir().mkpath(outputFolder);
    QString base = QFileInfo(imageName).completeBaseName();
    QFile f(QDir(outputFolder).filePath(base + ".txt"));
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << "ID | IMG | RAW | TRAD FR | STATUT | TRADUCTEUR | NOTE\n";

    for (const auto& b : blocks) {
        out << QString("%1 | %2 | %3 | %4 | %5 | %6 | %7\n")
                   .arg(b.id, 3, 10, QChar('0'))
                   .arg(imageName)
                   .arg(b.originalText.simplified())
                   .arg(b.translatedText.simplified())
                   .arg(b.status)
                   .arg(b.translator)
                   .arg(b.notes.simplified());
    }
    f.close();
    qDebug() << "Exporter: TXT →" << f.fileName();
    return true;
}

// ─── JSON standard par image ──────────────────────────────────────────────────
bool Exporter::exportJSON(const std::vector<TextBlock>& blocks,
                           const QString& imageName,
                           const QString& filePath)
{
    QJsonObject root;
    root["image"]   = imageName;
    root["version"] = 2;

    QJsonArray arr;
    for (const auto& b : blocks)
        arr.append(b.toJson());
    root["blocks"] = arr;

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    qDebug() << "Exporter: JSON →" << filePath;
    return true;
}

// ─── JSON Photoshop ───────────────────────────────────────────────────────────
bool Exporter::exportPhotoshopJSON(const std::vector<TextBlock>& blocks,
                                    const QString& imageName,
                                    const QString& filePath)
{
    QJsonObject root;
    root["image"]   = imageName;
    root["version"] = 1;

    QJsonArray arr;
    for (const auto& b : blocks) {
        QJsonObject obj;
        obj["id"]     = b.id;
        obj["raw"]    = b.originalText;
        obj["trad"]   = b.translatedText;
        obj["status"] = b.status;
        obj["notes"]  = b.notes;

        // Coordonnées rectangle externe
        QJsonObject rect;
        rect["x"] = b.boundingBox.x();
        rect["y"] = b.boundingBox.y();
        rect["w"] = b.boundingBox.width();
        rect["h"] = b.boundingBox.height();
        obj["rect"] = rect;

        // Coordonnées rectangle interne (zone texte PS)
        QJsonObject ir;
        ir["x"] = b.innerRect.x();
        ir["y"] = b.innerRect.y();
        ir["w"] = b.innerRect.width();
        ir["h"] = b.innerRect.height();
        obj["inner_rect"] = ir;

        // Centre (pratique pour le plugin PS)
        QJsonObject center;
        center["cx"] = b.boundingBox.x() + b.boundingBox.width()  / 2;
        center["cy"] = b.boundingBox.y() + b.boundingBox.height() / 2;
        obj["center"] = center;

        arr.append(obj);
    }
    root["bubbles"] = arr;

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    qDebug() << "Exporter: JSON Photoshop →" << filePath;
    return true;
}

// ─── PNG rendu ────────────────────────────────────────────────────────────────
bool Exporter::exportPNG(const QImage& base,
                          const std::vector<TextBlock>& blocks,
                          const QString& filePath)
{
    if (base.isNull()) return false;

    QImage result = base.convertToFormat(QImage::Format_ARGB32);
    QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing);
    QFont font("Arial", 10, QFont::Bold);
    p.setFont(font);

    for (const auto& b : blocks) {
        const QString& text = b.translatedText.isEmpty()
                                  ? b.originalText : b.translatedText;
        if (text.isEmpty()) continue;

        // Efface la bulle (blanc)
        p.setBrush(Qt::white);
        p.setPen(Qt::NoPen);
        if (!b.polygon.isEmpty())
            p.drawPolygon(b.polygon);
        else
            p.drawRect(b.boundingBox);

        // Écrit le texte dans l'inner rect
        p.setPen(Qt::black);
        p.drawText(b.innerRect, Qt::AlignCenter | Qt::TextWordWrap, text);
    }
    p.end();

    bool ok = result.save(filePath, "PNG");
    qDebug() << "Exporter: PNG →" << filePath << (ok ? "OK" : "ERREUR");
    return ok;
}

// ─── Fichier consolidé ────────────────────────────────────────────────────────
bool Exporter::exportConsolidated(const QString& outputFolder,
                                   const QString& destFile)
{
    QFile out(destFile);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream stream(&out);
    stream.setEncoding(QStringConverter::Utf8);

    // Header statuts
    stream << "| Code | Signification              |\n";
    stream << "| ---- | -------------------------- |\n";
    stream << "| TODO | Pas encore traduit         |\n";
    stream << "| WIP  | En cours de traduction     |\n";
    stream << "| TL   | Traduit (première version) |\n";
    stream << "| REV  | En relecture               |\n";
    stream << "| OK   | Validé version finale      |\n";
    stream << "| FIX  | Correction demandée        |\n\n";
    stream << "ID | IMG | RAW | TRAD FR | STATUT | TRADUCTEUR | NOTE\n";

    int globalId = 1;
    QStringList files;
    QDirIterator it(outputFolder, {"*.txt"}, QDir::Files);
    while (it.hasNext()) files << it.next();
    files.sort();

    for (const QString& path : files) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        QTextStream in(&f);
        in.setEncoding(QStringConverter::Utf8);
        bool firstLine = true;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (firstLine) { firstLine = false; continue; } // skip header
            if (line.isEmpty()) continue;
            QStringList parts = line.split("|");
            if (parts.size() >= 1)
                parts[0] = QString("%1").arg(globalId++, 3, 10, QChar('0'));
            stream << parts.join("|") << "\n";
        }
        f.close();
    }

    out.close();
    qDebug() << "Exporter: consolidé →" << destFile;
    return true;
}
