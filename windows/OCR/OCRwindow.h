#ifndef OCRWINDOW_H
#define OCRWINDOW_H

#include <QMainWindow>
#include "../../core/OCRManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class OCRwindow; }
QT_END_NAMESPACE

class OCRwindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit OCRwindow(QWidget *parent = nullptr);
    ~OCRwindow();
    void addOCRBlock(int id, QString original);

private slots:
    void loadTestImage();

private:
    Ui::OCRwindow *ui;
};

#endif