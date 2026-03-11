#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <QString>
#include <vector>
#include "TextBlock.h"


class Translator
{
public:
    Translator() = default;

    QString translate(const QString& text,
                      const QString& sourceLang = "en",
                      const QString& targetLang = "fr");

    void translateAll(std::vector<TextBlock>& blocks,
                      const QString& sourceLang = "en",
                      const QString& targetLang = "fr");

private:
    QString translateViaDeepL(const QString& text,
                               const QString& source,
                               const QString& target);
};

#endif // TRANSLATOR_H
