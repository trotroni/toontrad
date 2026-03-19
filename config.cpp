#include "config.h"
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

QString   Config::pythonBin      = "python3";
QString   Config::detectScript   = "";
QString   Config::tessdataPath   = "";
bool      Config::darkMode       = false;
QString   Config::deeplApiKey    = "";
double    Config::innerRectRatio = 0.85;
OCRConfig Config::ocrConfig      = {};

void Config::load()
{
    QSettings s("ToonTrad", "ToonTrad");
    pythonBin      = s.value("pythonBin",      "python3").toString();
    darkMode       = s.value("darkMode",       false).toBool();
    deeplApiKey    = s.value("deeplApiKey",    "").toString();
    innerRectRatio = s.value("innerRectRatio", 0.85).toDouble();

    // Résolution automatique de detect.py
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList scriptCandidates = {
        appDir + "/python/detect.py",
        appDir + "/../python/detect.py",
        appDir + "/../../python/detect.py",
        appDir + "/../../../python/detect.py",
        QDir::currentPath() + "/python/detect.py",
    };
    for (const QString& c : scriptCandidates) {
        if (QFile::exists(c)) { detectScript = QDir::cleanPath(c); break; }
    }
    if (detectScript.isEmpty())
        detectScript = s.value("detectScript", "python/detect.py").toString();

    // Résolution tessdata
    QStringList tessdataCandidates = {
        appDir + "/resources/tessdata",
        appDir + "/../resources/tessdata",
        appDir + "/../../resources/tessdata",
        appDir + "/../../../resources/tessdata",
        QDir::currentPath() + "/resources/tessdata",
    };
    for (const QString& c : tessdataCandidates) {
        if (QDir(c).exists()) { tessdataPath = QDir::cleanPath(c); break; }
    }
    if (tessdataPath.isEmpty())
        tessdataPath = s.value("tessdataPath", "").toString();

    // Charge OCRConfig persistant
    ocrConfig = loadOCR();
}

void Config::save()
{
    QSettings s("ToonTrad", "ToonTrad");
    s.setValue("pythonBin",      pythonBin);
    s.setValue("darkMode",       darkMode);
    s.setValue("deeplApiKey",    deeplApiKey);
    s.setValue("innerRectRatio", innerRectRatio);
    s.setValue("detectScript",   detectScript);
    s.setValue("tessdataPath",   tessdataPath);
}

void Config::saveOCR(const OCRConfig& c)
{
    QSettings s("ToonTrad", "ToonTrad");
    s.setValue("ocr/engine",               c.engine);
    s.setValue("ocr/device",               c.device);
    s.setValue("ocr/gpuId",               c.gpuId);
    s.setValue("ocr/gpuMemFraction",       c.gpuMemFraction);
    s.setValue("ocr/ramFraction",          c.ramFraction);
    s.setValue("ocr/ramGb",               c.ramGb);
    s.setValue("ocr/confidenceThreshold",  c.confidenceThreshold);
    s.setValue("ocr/minBubbleArea",        c.minBubbleArea);
    s.setValue("ocr/minTextLen",           c.minTextLen);
    s.setValue("ocr/language",             c.language);
    s.setValue("ocr/psmMode",             c.psmMode);
    ocrConfig = c;
}

OCRConfig Config::loadOCR()
{
    QSettings s("ToonTrad", "ToonTrad");
    OCRConfig c;
    c.engine              = s.value("ocr/engine",              "auto").toString();
    c.device              = s.value("ocr/device",              "auto").toString();
    c.gpuId               = s.value("ocr/gpuId",              0).toInt();
    c.gpuMemFraction      = s.value("ocr/gpuMemFraction",      0.5).toDouble();
    c.ramFraction         = s.value("ocr/ramFraction",         0.5).toDouble();
    c.ramGb               = s.value("ocr/ramGb",              4).toInt();
    c.confidenceThreshold = s.value("ocr/confidenceThreshold", 0.4).toDouble();
    c.minBubbleArea       = s.value("ocr/minBubbleArea",       2000).toInt();
    c.minTextLen          = s.value("ocr/minTextLen",          2).toInt();
    c.language            = s.value("ocr/language",            "en").toString();
    c.psmMode             = s.value("ocr/psmMode",            6).toInt();
    return c;
}
