#ifndef OCRCONFIG_H
#define OCRCONFIG_H

#include <QString>
#include <QJsonObject>

struct OCRConfig
{
    QString engine             = "auto";
    QString device             = "auto";
    int     gpuId              = 0;
    double  gpuMemFraction     = 0.5;
    double  ramFraction        = 0.5;
    int     ramGb              = 4;       // valeur absolue GB (sync sliders MainWindow ↔ Settings)
    double  confidenceThreshold = 0.4;
    int     minBubbleArea      = 2000;
    int     minTextLen         = 3;
    QString language           = "en";
    int     psmMode            = 6;

    QJsonObject toJson() const {
        QJsonObject o;
        o["engine"]               = engine;
        o["device"]               = device;
        o["gpu_id"]               = gpuId;
        o["gpu_mem"]              = gpuMemFraction;
        o["ram_fraction"]         = ramFraction;
        o["confidence_threshold"] = confidenceThreshold;
        o["min_bubble_area"]      = minBubbleArea;
        o["min_text_len"]         = minTextLen;
        o["language"]             = language;
        o["psm_mode"]             = psmMode;
        o["inner_ratio"]          = 0.85;
        // tessdata_path injecté par OCRManager depuis Config::tessdataPath
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
        if (o.contains("min_text_len"))         c.minTextLen          = o["min_text_len"].toInt();
        if (o.contains("language"))             c.language            = o["language"].toString();
        if (o.contains("psm_mode"))             c.psmMode             = o["psm_mode"].toInt();
        return c;
    }
};

#endif // OCRCONFIG_H