#ifndef BUBBLE_H
#define BUBBLE_H

#include <QString>
#include <QRect>
#include <QJsonObject>

// Statuts identiques à la version Python
inline const QStringList STATUTS = {"TODO", "WIP", "TL", "REV", "OK", "FIX"};

struct Bubble
{
    int     id     = 0;
    QRect   rect;        // rectangle externe  (x, y, w, h)
    QRect   innerRect;   // rectangle interne  (calculé via innerRectRatio)

    QString raw;         // texte OCR brut
    QString trad;        // traduction
    QString status = "TODO";

    // ── Constructeurs ──────────────────────────────────────────────────────

    Bubble() = default;

    Bubble(int id, QRect rect, QString raw = "", QString trad = "",
           QString status = "TODO")
        : id(id), rect(rect), raw(std::move(raw)),
          trad(std::move(trad)), status(std::move(status))
    {
        innerRect = computeInnerRect(rect);
    }

    // ── Calcul inner rect (même logique que Python) ────────────────────────

    static QRect computeInnerRect(const QRect& r);

    void updateRect(const QRect& newRect)
    {
        rect      = newRect;
        innerRect = computeInnerRect(newRect);
    }

    // ── JSON (pour communication QProcess ↔ Python) ────────────────────────

    QJsonObject toJson() const;
    static Bubble fromJson(const QJsonObject& obj);
};

#endif // BUBBLE_H