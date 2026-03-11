#ifndef OCRMANAGER_H
#define OCRMANAGER_H

#include <QObject>
#include <QProcess>
#include <QImage>
#include <vector>
#include "TextBlock.h"
#include "OCRConfig.h"

class OCRManager : public QObject
{
    Q_OBJECT

public:
    explicit OCRManager(QObject* parent = nullptr);

    std::vector<TextBlock> runOCR(const QString& imagePath,
                                  const OCRConfig& config);

    static bool checkPythonAvailable(QString* errorMsg = nullptr);

signals:
    void errorOccurred(const QString& message);

private:
    std::vector<TextBlock> parseOutput(const QByteArray& json);
};

#endif // OCRMANAGER_H
