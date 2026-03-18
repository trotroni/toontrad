#ifndef BUBBLEDETECTOR_H
#define BUBBLEDETECTOR_H

#include <QObject>
#include <QProcess>
#include <vector>
#include "Bubble.h"

class BubbleDetector : public QObject
{
    Q_OBJECT

public:
    explicit BubbleDetector(QObject* parent = nullptr);

    // Lance detect.py sur l'image et retourne les bulles parsées.
    // Bloquant (waitForFinished) — appelé depuis un thread UI uniquement
    // si l'image n'est pas trop lourde, sinon utiliser runAsync.
    std::vector<Bubble> run(const QString& imagePath);

    // Vérification rapide : python3 + detect.py accessibles ?
    static bool checkAvailable(QString* errorMsg = nullptr);

    signals:
        void errorOccurred(const QString& message);

private:
    std::vector<Bubble> parseJson(const QByteArray& json);
};

#endif // BUBBLEDETECTOR_H