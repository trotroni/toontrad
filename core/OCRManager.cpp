#include "OCRManager.h"
#include "../config.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QFile>

OCRManager::OCRManager(QObject* parent) : QObject(parent) {}


bool OCRManager::checkPythonAvailable(QString* errorMsg)
{
    QProcess p;
    p.start(Config::pythonBin, {"--version"});
    if (!p.waitForFinished(5000)) {
        if (errorMsg) *errorMsg = "Python introuvable : " + Config::pythonBin;
        return false;
    }
    if (!QFile::exists(Config::pythonScript)) {
        if (errorMsg) *errorMsg = "Script introuvable : " + Config::pythonScript;
        return false;
    }
    return true;
}


std::vector<TextBlock> OCRManager::runOCR(const QString& imagePath,
                                           const OCRConfig& config)
{
    QJsonObject args = config.toJson();
    args["image_path"] = imagePath;

    QString argsJson = QJsonDocument(args).toJson(QJsonDocument::Compact);

    qDebug() << "OCRManager: lancement Python"
             << Config::pythonBin << Config::pythonScript;
    qDebug() << "Args:" << argsJson;

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(Config::pythonBin, {Config::pythonScript, argsJson});

    if (!process.waitForStarted(10000)) {
        emit errorOccurred("Impossible de démarrer Python : " + Config::pythonBin);
        return {};
    }

    if (!process.waitForFinished(120000)) {
        process.kill();
        emit errorOccurred("Timeout OCR (120s dépassé)");
        return {};
    }

    QByteArray stderrData = process.readAllStandardError();
    if (!stderrData.isEmpty())
        qDebug() << "Python stderr:" << stderrData;

    if (process.exitCode() != 0) {
        emit errorOccurred("Erreur Python (code " +
                           QString::number(process.exitCode()) + "): " +
                           QString::fromUtf8(stderrData));
        return {};
    }

    QByteArray output = process.readAllStandardOutput();
    qDebug() << "OCRManager: output reçu (" << output.size() << "bytes)";

    return parseOutput(output);
}


std::vector<TextBlock> OCRManager::parseOutput(const QByteArray& json)
{
    std::vector<TextBlock> blocks;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError) {
        emit errorOccurred("Erreur JSON Python : " + err.errorString());
        return blocks;
    }

    if (!doc.isArray()) {
        emit errorOccurred("Réponse Python invalide (pas un tableau JSON)");
        return blocks;
    }

    int id = 1;
    for (const QJsonValue& v : doc.array()) {
        QJsonObject obj = v.toObject();

        QString text       = obj["text"].toString();
        double  confidence = obj["confidence"].toDouble(1.0);

        QPolygon poly;
        for (const QJsonValue& pt : obj["box"].toArray()) {
            QJsonArray p = pt.toArray();
            if (p.size() >= 2)
                poly << QPoint(p[0].toInt(), p[1].toInt());
        }

        if (!text.isEmpty() && !poly.isEmpty())
            blocks.emplace_back(id++, poly, text, confidence);
    }

    qDebug() << "OCRManager:" << blocks.size() << "blocs parsés";
    return blocks;
}
