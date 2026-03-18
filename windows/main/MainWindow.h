#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPointer>
#include <QMap>
#include <QNetworkAccessManager>
#include "../../core/OCRConfig.h"
#include "../image/ImageWindow.h"
#include "../text/TextWindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    OCRConfig currentConfig() const;

private slots:
    void on_btnNewProject_clicked();
    void on_btnRemoveProject_clicked();
    void on_btnOpenProject_clicked();
    void on_btnSettings_clicked();
    void on_listProjects_itemDoubleClicked();
    void on_listProjects_currentRowChanged(int row);
    void on_comboEngine_currentIndexChanged(int index);
    void on_comboDevice_currentIndexChanged(int index);
    void on_sliderVRAM_valueChanged(int value);
    void on_sliderRAM_valueChanged(int value);

private:
    Ui::MainWindow* ui;
    OCRConfig       m_config;

    // Fenêtres ouvertes par projet : path → {ImageWindow, TextWindow}
    struct ProjectWindows {
        QPointer<ImageWindow> imageWin;
        QPointer<TextWindow>  textWin;
    };
    QMap<QString, ProjectWindows> m_openWindows;

    QLabel*               m_lblVersion = nullptr;
    QNetworkAccessManager m_network;

    void refreshProjectList();
    void detectGPUs();
    void openSelectedProject();
    void checkLatestVersion();
    void runPipUpdate();
};

#endif // MAINWINDOW_H
