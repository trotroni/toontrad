#ifndef CONFIG_H
#define CONFIG_H

#include <QString>
#include "core/OCRConfig.h"

class Config
{
public:
    inline static const QString VERSION_STR = "1.0.0";

    // Python
    static QString pythonBin;
    static QString detectScript;
    static QString tessdataPath;

    // App
    static bool    darkMode;
    static QString deeplApiKey;

    // Inner rect ratio
    static double  innerRectRatio;

    // Paramètres OCR persistants
    static OCRConfig ocrConfig;

    static void load();
    static void save();
    static void saveOCR(const OCRConfig& config);
    static OCRConfig loadOCR();
};

#endif // CONFIG_H
