#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QPointer>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QLabel>
#include <QEvent>
#include "../../core/OCRConfig.h"
#include "../project/NewProjectDialog.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // openPath : chemin vers un .ttproject passé en argument (double-clic OS).
    // Laisser vide pour un démarrage normal.
    explicit MainWindow(const QString& openPath = {}, QWidget* parent = nullptr);
    ~MainWindow();

    OCRConfig currentConfig() const;

    // Ouvre le projet dont le fichier .ttproject est à <filePath>.
    // Appelé depuis main() (Windows/Linux) ou depuis event() (macOS bundle).
    void openProjectFromPath(const QString& filePath);

protected:
    // Intercepte QFileOpenEvent envoyé par macOS quand on double-clique
    // sur un .ttproject associé au bundle.
    bool event(QEvent* e) override;

private slots:
    void on_btnNewProject_clicked();
    void on_btnRemoveProject_clicked();
    void on_btnOpenProject_clicked();
    void on_btnSettings_clicked();
    void on_btnReloadProject_clicked();
    void on_listProjects_itemDoubleClicked();
    void on_listProjects_currentRowChanged(int row);
    void on_sliderVRAM_valueChanged(int value);
    void on_sliderRAM_valueChanged(int value);
    void on_comboDevice_currentIndexChanged(int index);

private:
    Ui::MainWindow* ui;
    OCRConfig       m_config;

    void refreshProjectList();
    void detectGPUs();
    void openSelectedProject();
    void openProject(const QString& rootPath);

    void checkLatestVersion();
    void runPipUpdate();

    QMap<QString, QPointer<QMainWindow>> m_openWindows;
    QLabel*               m_lblVersion = nullptr;
    QNetworkAccessManager m_network;
};

#endif // MAINWINDOW_H
