#include "Exporter.h"
#include <QPainter>
#include <QFont>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTextStream>
#include <QDebug>

QImage Exporter::renderImage(const QImage& base,
                              const std::vector<TextBlock>& blocks)
{
    if (base.isNull()) return {};

    QImage result = base.convertToFormat(QImage::Format_ARGB32);
    QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing);

    QFont font("Arial", 10, QFont::Bold);
    p.setFont(font);

    for (const auto& b : blocks) {
        const QString& text = b.translatedText.isEmpty()
                                  ? b.originalText : b.translatedText;
        if (text.isEmpty()) continue;

        p.setBrush(Qt::white);
        p.setPen(Qt::NoPen);
        if (!b.polygon.isEmpty())
            p.drawPolygon(b.polygon);
        else
            p.drawRect(b.boundingBox);

        p.setPen(Qt::black);
        p.drawText(b.boundingBox, Qt::AlignCenter | Qt::TextWordWrap, text);
    }
    p.end();
    return result;
}


bool Exporter::exportJSON(const std::vector<TextBlock>& blocks,
                           const QString& imageName,
                           const QString& filePath)
{
    QJsonObject root;
    root["image"] = imageName;
    root["version"] = 2;

    QJsonArray arr;
    for (const auto& b : blocks) {
        QJsonObject obj = b.toJson();
        QJsonObject ps;

        ps["x"] = b.boundingBox.x();
        ps["y"] = b.boundingBox.y();
        ps["width"]  = b.boundingBox.width();
        ps["height"] = b.boundingBox.height();

        ps["cx"] = b.boundingBox.x() + b.boundingBox.width()  / 2;
        ps["cy"] = b.boundingBox.y() + b.boundingBox.height() / 2;
        obj["photoshop"] = ps;
        arr.append(obj);
    }
    root["blocks"] = arr;

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    qDebug() << "Exporter: JSON →" << filePath;
    return true;
}


bool Exporter::exportTXT(const std::vector<TextBlock>& blocks,
                          const QString& imageName,
                          const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;

    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << "ID | IMG | RAW | TRAD FR | STATUT | CONF | X | Y | W | H | POLYGONE | TRADUCTEUR | NOTE\n";

    for (const auto& b : blocks) {
        QString raw    = b.originalText.simplified();
        QString trad   = b.translatedText.simplified();
        QString status = trad.isEmpty() ? "TODO" : "DONE";

        QStringList pts;
        for (const QPoint& p : b.polygon)
            pts << QString("%1,%2").arg(p.x()).arg(p.y());

        out << QString("%1 | %2 | %3 | %4 | %5 | %6 | %7 | %8 | %9 | %10 | %11 |  | \n")
                   .arg(b.id, 3, 10, QChar('0'))
                   .arg(imageName)
                   .arg(raw)
                   .arg(trad)
                   .arg(status)
                   .arg(b.confidence, 0, 'f', 2)
                   .arg(b.boundingBox.x())
                   .arg(b.boundingBox.y())
                   .arg(b.boundingBox.width())
                   .arg(b.boundingBox.height())
                   .arg(pts.join(";"));
    }

    f.close();
    qDebug() << "Exporter: TXT →" << filePath;
    return true;
}
