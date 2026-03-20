#include "BubbleDetector.h"
#include "../config.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDebug>

BubbleDetector::BubbleDetector(QObject* parent) : QObject(parent) {}

bool BubbleDetector::checkAvailable(QString* errorMsg)
{
    QProcess p;
    p.start(Config::pythonBin, {"--version"});
    if (!p.waitForFinished(5000)) {
        if (errorMsg) *errorMsg = "Python introuvable : " + Config::pythonBin;
        return false;
    }
    if (!QFile::exists(Config::detectScript)) {
        if (errorMsg) *errorMsg = "Script introuvable : " + Config::detectScript;
        return false;
    }
    return true;
}

std::vector<TextBlock> BubbleDetector::run(const QString& imagePath,
                                            const OCRConfig& config)
{
    // Construit le JSON d'arguments pour detect.py
    QJsonObject args = config.toJson();
    args["image_path"]    = imagePath;
    args["inner_ratio"]   = Config::innerRectRatio;
    args["tessdata_path"] = Config::tessdataPath;

    QString argsJson = QJsonDocument(args).toJson(QJsonDocument::Compact);

    qDebug() << "BubbleDetector: lancement" << Config::pythonBin << Config::detectScript;

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(Config::pythonBin, {Config::detectScript, argsJson});

    if (!process.waitForStarted(10000)) {
        emit errorOccurred("Impossible de démarrer Python : " + Config::pythonBin);
        return {};
    }
    if (!process.waitForFinished(120000)) {
        process.kill();
        emit errorOccurred("Timeout détection (120s)");
        return {};
    }

    QByteArray stderrData = process.readAllStandardError();
    if (!stderrData.isEmpty())
        qDebug() << "detect.py stderr:" << stderrData;

    if (process.exitCode() != 0) {
        emit errorOccurred("Erreur Python (code " +
                           QString::number(process.exitCode()) + ") :\n" +
                           QString::fromUtf8(stderrData));
        return {};
    }

    return parseOutput(process.readAllStandardOutput());
}

std::vector<TextBlock> BubbleDetector::parseOutput(const QByteArray& json)
{
    std::vector<TextBlock> blocks;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError) {
        emit errorOccurred("Erreur JSON : " + err.errorString());
        return blocks;
    }
    if (!doc.isArray()) {
        emit errorOccurred("Réponse Python invalide (pas un tableau)");
        return blocks;
    }

    int id = 1;
    for (const QJsonValue& v : doc.array()) {
        QJsonObject obj = v.toObject();

        QString text = obj["raw"].toString();
        if (text.isEmpty()) text = obj["text"].toString();
        double conf  = obj["confidence"].toDouble(0.9);

        QJsonArray r = obj["rect"].toArray();
        if (r.size() < 4) continue;

        QRect rect(r[0].toInt(), r[1].toInt(), r[2].toInt(), r[3].toInt());
        TextBlock b(id++, rect, text, conf);

        // inner_rect depuis Python si présent
        if (obj.contains("inner_rect")) {
            QJsonArray ir = obj["inner_rect"].toArray();
            if (ir.size() == 4)
                b.innerRect = QRect(ir[0].toInt(), ir[1].toInt(),
                                    ir[2].toInt(), ir[3].toInt());
        }

        blocks.push_back(b);
    }

    qDebug() << "BubbleDetector:" << blocks.size() << "blocs parsés";
    return blocks;
}