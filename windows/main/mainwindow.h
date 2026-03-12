#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QPointer>
#include <QMap>
#include "../../core/OCRConfig.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    OCRConfig currentConfig() const;

private slots:
    void on_btnNewProject_clicked();
    void on_btnRemoveProject_clicked();
    void on_btnOpenProject_clicked();
    void on_btnSettings_clicked();
    void on_listProjects_itemDoubleClicked();
    void on_listProjects_currentRowChanged(int row);
    void on_sliderVRAM_valueChanged(int value);
    void on_sliderRAM_valueChanged(int value);
    void on_comboDevice_currentIndexChanged(int index);

private:
    Ui::MainWindow* ui;
    OCRConfig m_config;

    void refreshProjectList();
    void detectGPUs();
    void applyTheme();
    void openSelectedProject();

    QMap<QString, QPointer<QMainWindow>> m_openWindows;
};

#endif // MAINWINDOW_H