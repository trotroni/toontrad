#ifndef BUBBLEDETECTOR_H
#define BUBBLEDETECTOR_H

#include <QObject>
#include <QProcess>
#include <vector>
#include "TextBlock.h"
#include "OCRConfig.h"

class BubbleDetector : public QObject
{
    Q_OBJECT

public:
    explicit BubbleDetector(QObject* parent = nullptr);

    // Lance detect.py → retourne les blocs parsés
    std::vector<TextBlock> run(const QString& imagePath, const OCRConfig& config);

    static bool checkAvailable(QString* errorMsg = nullptr);

signals:
    void errorOccurred(const QString& message);

private:
    std::vector<TextBlock> parseOutput(const QByteArray& json);
};

#endif // BUBBLEDETECTOR_H
