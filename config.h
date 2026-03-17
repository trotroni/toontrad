#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

class Config
{
public:
    inline static const QString VERSION_STR = "v.0.0.1-debug";

    static QString pythonBin;
    static QString pythonScript;

    static bool darkMode;

    static QString deeplApiKey;

    static void load();
    static void save();
};

#endif // CONFIG_H
