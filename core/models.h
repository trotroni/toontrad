#ifndef MODELS_H
#define MODELS_H

#include <QString>
#include <QStringList>

namespace ToonTrad {

inline QStringList engineList() {
    return {"auto", "paddleocr", "easyocr", "trocr", "manga-ocr", "tesseract"};
}

inline QStringList engineDisplayNames() {
    return {"Auto (détection matériel)", "PaddleOCR", "EasyOCR",
            "TrOCR (Microsoft)", "manga-ocr", "Tesseract (défaut CPU)"};
}

inline QStringList languageList() {
    return {"en", "ja", "zh", "fr", "de", "es", "ko"};
}

inline QStringList languageDisplayNames() {
    return {"Anglais", "Japonais", "Chinois", "Français",
            "Allemand", "Espagnol", "Coréen"};
}

inline QStringList statusList() {
    return {"TODO", "WIP", "TL", "REV", "OK", "FIX"};
}

}

#endif // MODELS_H
