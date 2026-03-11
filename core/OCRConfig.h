#ifndef OCRCONFIG_H
#define OCRCONFIG_H

#include <QString>
#include <QJsonObject>

struct OCRConfig
{
    QString engine             = "auto";  // auto|paddleocr|easyocr|trocr|manga-ocr|tesseract

    QString device             = "auto";  // auto|cpu|cuda:0|cuda:1|...
    int     gpuId              = 0;
    double  gpuMemFraction     = 0.5;     // 0.1 – 1.0
    double  ramFraction        = 0.5;     // fraction RAM système

    double  confidenceThreshold = 0.4;   // 0.0 – 1.0
    int     minBubbleArea       = 2000;  // px²
    QString language            = "en";  // en|ja|fr|...
    int     psmMode             = 6;     // Tesseract PSM (3=auto, 6=bloc, 11=sparse)

    QJsonObject toJson() const {
        QJsonObject o;
        o["engine"]               = engine;
        o["device"]               = device;
        o["gpu_id"]               = gpuId;
        o["gpu_mem"]              = gpuMemFraction;
        o["ram_fraction"]         = ramFraction;
        o["confidence_threshold"] = confidenceThreshold;
        o["min_bubble_area"]      = minBubbleArea;
        o["language"]             = language;
        o["psm_mode"]             = psmMode;
        return o;
    }

    static OCRConfig fromJson(const QJsonObject& o) {
        OCRConfig c;
        if (o.contains("engine"))               c.engine              = o["engine"].toString();
        if (o.contains("device"))               c.device              = o["device"].toString();
        if (o.contains("gpu_id"))               c.gpuId               = o["gpu_id"].toInt();
        if (o.contains("gpu_mem"))              c.gpuMemFraction      = o["gpu_mem"].toDouble();
        if (o.contains("ram_fraction"))         c.ramFraction         = o["ram_fraction"].toDouble();
        if (o.contains("confidence_threshold")) c.confidenceThreshold = o["confidence_threshold"].toDouble();
        if (o.contains("min_bubble_area"))      c.minBubbleArea       = o["min_bubble_area"].toInt();
        if (o.contains("language"))             c.language            = o["language"].toString();
        if (o.contains("psm_mode"))             c.psmMode             = o["psm_mode"].toInt();
        return c;
    }
};

#endif // OCRCONFIG_H
