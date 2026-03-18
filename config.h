#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

class Config
{
public:
    inline static const QString VERSION_STR = "0.0.1-debug";

    // Python
    static QString pythonBin;
    static QString detectScript;   // chemin vers python/detect.py

    // App
    static bool    darkMode;
    static QString deeplApiKey;

    // Inner rect ratio (zone de texte dans la bulle, 0.0–1.0)
    static double  innerRectRatio;

    static void load();
    static void save();
};

#endif // CONFIG_H
