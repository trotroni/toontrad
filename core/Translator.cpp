#include "Translator.h"
#include "../config.h"
#include <QDebug>

QString Translator::translateViaDeepL(const QString& text,
                                       const QString& source,
                                       const QString& target)
{
    Q_UNUSED(source); Q_UNUSED(target);

    qDebug() << "Translator: API non implémentée — traduction manuelle requise";
    return text;
}

QString Translator::translate(const QString& text,
                               const QString& sourceLang,
                               const QString& targetLang)
{
    if (text.trimmed().isEmpty()) return {};
    if (!Config::deeplApiKey.isEmpty())
        return translateViaDeepL(text, sourceLang, targetLang);
    return text;
}

void Translator::translateAll(std::vector<TextBlock>& blocks,
                               const QString& sourceLang,
                               const QString& targetLang)
{
    for (auto& b : blocks)
        b.translatedText = translate(b.originalText, sourceLang, targetLang);
}
