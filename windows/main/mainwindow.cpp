#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../OCR/OCRwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_btnOpenOCR_clicked()
{
    OCRwindow *ocr = new OCRwindow(this);
    ocr->show();
}