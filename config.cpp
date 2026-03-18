#include "config.h"
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

QString Config::pythonBin      = "python3";
QString Config::detectScript   = "";
bool    Config::darkMode       = false;
QString Config::deeplApiKey    = "";
double  Config::innerRectRatio = 0.85;

void Config::load()
{
    QSettings s("ToonTrad", "ToonTrad");
    pythonBin      = s.value("pythonBin",      "python3").toString();
    darkMode       = s.value("darkMode",       false).toBool();
    deeplApiKey    = s.value("deeplApiKey",    "").toString();
    innerRectRatio = s.value("innerRectRatio", 0.85).toDouble();

    // Résolution automatique de detect.py
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + "/python/detect.py",
        appDir + "/../python/detect.py",
        appDir + "/../../python/detect.py",
        QDir::currentPath() + "/python/detect.py",
    };
    for (const QString& c : candidates) {
        if (QFile::exists(c)) {
            detectScript = QDir::cleanPath(c);
            break;
        }
    }
    if (detectScript.isEmpty())
        detectScript = s.value("detectScript", "python/detect.py").toString();
}

void Config::save()
{
    QSettings s("ToonTrad", "ToonTrad");
    s.setValue("pythonBin",      pythonBin);
    s.setValue("darkMode",       darkMode);
    s.setValue("deeplApiKey",    deeplApiKey);
    s.setValue("innerRectRatio", innerRectRatio);
    s.setValue("detectScript",   detectScript);
}
