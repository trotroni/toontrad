#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../OCR/OCRwindow.h"
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //zone de texte /textBrowser
    ui->textBrowser->append("Projet chargé ✓");
    ui->textBrowser->append("OCR lancé sur img_001...");
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

void MainWindow::on_buttonBox_accepted()
{
    OCRwindow *ocr = new OCRwindow(this);
    ocr->show();
}

void MainWindow::on_buttonBox_rejected()
{
    QApplication::quit();
}

void MainWindow::on_githubRepoButton_clicked()
{
    QDesktopServices::openUrl(QUrl("https://github.com/trotroni/toontrad"));
}

void MainWindow::on_githubWebButton_clicked()
{
    QDesktopServices::openUrl(QUrl("https://trotroni.github.io/toontrad/"));
}

void MainWindow::on_githubDocsButton_clicked()
{
    QDesktopServices::openUrl(QUrl("https://github.com/trotroni/toontrad/wiki"));
}

