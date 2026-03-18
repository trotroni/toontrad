#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include <QStringList>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QFile>

class Config
{
public:
    // ── Mode ──────────────────────────────────────────────────────────────
    // "image" → une seule image | "folder" → dossier entier
    inline static QString mode         = "folder";
    inline static QString imagePath    = "";
    inline static QString folderPath   = "";

    // ── Output ────────────────────────────────────────────────────────────
    inline static QString outputFolder = "output";

    // ── Inner rect ────────────────────────────────────────────────────────
    // Ratio du rectangle interne par rapport au rectangle externe (0.0 – 1.0)
    inline static double innerRectRatio = 0.85;

    // ── Python ────────────────────────────────────────────────────────────
    inline static QString pythonBin    = "python3";
    inline static QString detectScript = "";   // résolu automatiquement au load()

    // ── OCR ───────────────────────────────────────────────────────────────
    inline static QString ocrLang      = "eng";
    inline static int     psmMode      = 6;
    inline static int     minArea      = 2000;
    inline static int     minTextLen   = 3;

    // ── Persistence ───────────────────────────────────────────────────────
    static void load()
    {
        QSettings s("ToonTrad", "ToonTrad");
        mode           = s.value("mode",           mode).toString();
        imagePath      = s.value("imagePath",      imagePath).toString();
        folderPath     = s.value("folderPath",     folderPath).toString();
        outputFolder   = s.value("outputFolder",   outputFolder).toString();
        innerRectRatio = s.value("innerRectRatio", innerRectRatio).toDouble();
        pythonBin      = s.value("pythonBin",      pythonBin).toString();
        ocrLang        = s.value("ocrLang",        ocrLang).toString();
        psmMode        = s.value("psmMode",        psmMode).toInt();
        minArea        = s.value("minArea",        minArea).toInt();
        minTextLen     = s.value("minTextLen",     minTextLen).toInt();

        // Résolution automatique du script detect.py
        resolveDetectScript();
    }

    static void save()
    {
        QSettings s("ToonTrad", "ToonTrad");
        s.setValue("mode",           mode);
        s.setValue("imagePath",      imagePath);
        s.setValue("folderPath",     folderPath);
        s.setValue("outputFolder",   outputFolder);
        s.setValue("innerRectRatio", innerRectRatio);
        s.setValue("pythonBin",      pythonBin);
        s.setValue("ocrLang",        ocrLang);
        s.setValue("psmMode",        psmMode);
        s.setValue("minArea",        minArea);
        s.setValue("minTextLen",     minTextLen);
    }

private:
    static void resolveDetectScript()
    {
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
                return;
            }
        }
        detectScript = "python/detect.py"; // fallback relatif
    }
};

#endif // CONFIG_H