#include "BubbleDetector.h"
#include "../config.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QDebug>

BubbleDetector::BubbleDetector(QObject* parent) : QObject(parent) {}

// ─────────────────────────────────────────────────────────────────────────────
//  Vérification disponibilité
// ─────────────────────────────────────────────────────────────────────────────

bool BubbleDetector::checkAvailable(QString* errorMsg)
{
    // 1. Python accessible ?
    QProcess p;
    p.start(Config::pythonBin, {"--version"});
    if (!p.waitForFinished(5000)) {
        if (errorMsg) *errorMsg = "Python introuvable : " + Config::pythonBin;
        return false;
    }

    // 2. Script detect.py présent ?
    if (!QFile::exists(Config::detectScript)) {
        if (errorMsg) *errorMsg = "Script introuvable : " + Config::detectScript;
        return false;
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Lancement détection
// ─────────────────────────────────────────────────────────────────────────────

std::vector<Bubble> BubbleDetector::run(const QString& imagePath)
{
    // Construit le JSON d'arguments (identique aux clés attendues par detect.py)
    QJsonObject args;
    args["image_path"]   = imagePath;
    args["lang"]         = Config::ocrLang;
    args["psm"]          = Config::psmMode;
    args["min_area"]     = Config::minArea;
    args["min_text_len"] = Config::minTextLen;
    args["inner_ratio"]  = Config::innerRectRatio;

    QString argsJson = QJsonDocument(args).toJson(QJsonDocument::Compact);

    qDebug() << "BubbleDetector: lancement" << Config::pythonBin
             << Config::detectScript << argsJson;

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(Config::pythonBin, {Config::detectScript, argsJson});

    if (!process.waitForStarted(10000)) {
        emit errorOccurred("Impossible de démarrer Python : " + Config::pythonBin);
        return {};
    }

    // Timeout 120s (images lourdes + OCR peuvent être lents)
    if (!process.waitForFinished(120000)) {
        process.kill();
        emit errorOccurred("Timeout détection (120s dépassé)");
        return {};
    }

    // Affiche stderr Python dans la console Qt (utile pour debug)
    QByteArray stderrData = process.readAllStandardError();
    if (!stderrData.isEmpty())
        qDebug() << "detect.py stderr:" << stderrData;

    if (process.exitCode() != 0) {
        emit errorOccurred(
            "Erreur Python (code " + QString::number(process.exitCode()) + ") :\n" +
            QString::fromUtf8(stderrData));
        return {};
    }

    QByteArray output = process.readAllStandardOutput();
    qDebug() << "BubbleDetector: réponse reçue (" << output.size() << "bytes)";

    return parseJson(output);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Parse JSON → vector<Bubble>
// ─────────────────────────────────────────────────────────────────────────────

std::vector<Bubble> BubbleDetector::parseJson(const QByteArray& json)
{
    std::vector<Bubble> bubbles;

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError) {
        emit errorOccurred("Erreur JSON Python : " + err.errorString());
        return bubbles;
    }

    if (!doc.isArray()) {
        emit errorOccurred("Réponse Python invalide (pas un tableau JSON)");
        return bubbles;
    }

    for (const QJsonValue& v : doc.array()) {
        Bubble b = Bubble::fromJson(v.toObject());
        if (b.id > 0 && !b.rect.isEmpty())
            bubbles.push_back(b);
    }

    qDebug() << "BubbleDetector:" << bubbles.size() << "bulles parsées";
    return bubbles;
}