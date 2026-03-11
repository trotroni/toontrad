#include "config.h"
#include <QSettings>
#include <QCoreApplication>
#include <QDir>

QString Config::pythonBin    = "python3";
QString Config::pythonScript = "";
bool    Config::darkMode     = false;
QString Config::deeplApiKey  = "";

void Config::load()
{
    QSettings s("ToonTrad", "ToonTrad");
    pythonBin    = s.value("pythonBin", "python3").toString();
    darkMode     = s.value("darkMode", false).toBool();
    deeplApiKey  = s.value("deeplApiKey", "").toString();

    QString appDir = QCoreApplication::applicationDirPath();
    QStringList candidates = {
        appDir + "/python/main_ocr.py",
        appDir + "/../python/main_ocr.py",
        appDir + "/../../python/main_ocr.py"
    };
    for (const QString& c : candidates) {
        if (QFile::exists(c)) { pythonScript = QDir::cleanPath(c); break; }
    }
    if (pythonScript.isEmpty())
        pythonScript = s.value("pythonScript", "python/main_ocr.py").toString();
}

void Config::save()
{
    QSettings s("ToonTrad", "ToonTrad");
    s.setValue("pythonBin",    pythonBin);
    s.setValue("darkMode",     darkMode);
    s.setValue("deeplApiKey",  deeplApiKey);
    s.setValue("pythonScript", pythonScript);
}
