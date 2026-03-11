#ifndef CONFIG_H
#define CONFIG_H

#include <QString>

class Config
{
public:
    static const int VERSION = 2;

    static QString pythonBin;
    static QString pythonScript;

    static bool darkMode;

    static QString deeplApiKey;

    static void load();
    static void save();
};

#endif // CONFIG_H
