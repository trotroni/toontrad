#include "config.h"
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

QString Config::pythonBin      = "python3";
QString Config::detectScript   = "";
QString Config::tessdataPath   = "";
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

    // Résolution automatique de detect.py et tessdata
    QString appDir = QCoreApplication::applicationDirPath();
    QStringList scriptCandidates = {
        appDir + "/python/detect.py",
        appDir + "/../python/detect.py",
        appDir + "/../../python/detect.py",
        QDir::currentPath() + "/python/detect.py",
    };
    for (const QString& c : scriptCandidates) {
        if (QFile::exists(c)) {
            detectScript = QDir::cleanPath(c);
            break;
        }
    }
    if (detectScript.isEmpty())
        detectScript = s.value("detectScript", "python/detect.py").toString();

    // tessdata — cherche resources/tessdata/ à côté de l'exe
    QStringList tessdataCandidates = {
        appDir + "/resources/tessdata",
        appDir + "/../resources/tessdata",           // dans le .app macOS
        appDir + "/../../resources/tessdata",
        QDir::currentPath() + "/resources/tessdata",
    };
    for (const QString& c : tessdataCandidates) {
        if (QDir(c).exists()) {
            tessdataPath = QDir::cleanPath(c);
            break;
        }
    }
    if (tessdataPath.isEmpty())
        tessdataPath = s.value("tessdataPath", "").toString();
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