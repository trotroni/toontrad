#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../../config.h"
#include "../../core/models.h"
#include "../../core/ProjectManager.h"
#include "../OCR/OCRwindow.h"
#include "../settings/SettingsWindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QApplication>
#include <QFile>
#include <QDebug>
#ifdef Q_OS_MAC
#include <sys/sysctl.h>
#include <QNetworkRequest>
#include <QJsonObject>
#include <QDir>
#include <QCoreApplication>

#endif
#ifdef Q_OS_LINUX
#include <sys/sysinfo.h>
#endif


static const char* DARK_THEME = R"(
QMainWindow, QDialog, QWidget { background-color: #1e1e2e; color: #cdd6f4; }
QGroupBox { border: 1px solid #45475a; border-radius: 6px; margin-top: 8px; padding-top: 8px; color: #cdd6f4; }
QGroupBox::title { subcontrol-origin: margin; left: 10px; }
QListWidget { background: #181825; border: 1px solid #45475a; border-radius: 4px; color: #cdd6f4; }
QListWidget::item:selected { background: #313244; }
QListWidget::item:hover { background: #292938; }
QPushButton { background: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; padding: 5px 10px; }
QPushButton:hover { background: #45475a; }
QPushButton:pressed { background: #585b70; }
QPushButton:disabled { color: #585b70; }
QComboBox { background: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; padding: 3px 6px; }
QComboBox QAbstractItemView { background: #1e1e2e; color: #cdd6f4; selection-background-color: #313244; }
QSlider::groove:horizontal { height: 4px; background: #45475a; border-radius: 2px; }
QSlider::handle:horizontal { background: #89b4fa; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; }
QSlider::sub-page:horizontal { background: #89b4fa; border-radius: 2px; }
QLabel { color: #cdd6f4; }
QScrollArea { border: none; }
QTextEdit { background: #181825; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; }
QLineEdit { background: #181825; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; padding: 3px; }
QSpinBox { background: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; }
QStatusBar { background: #181825; color: #888; }
QFrame[frameShape="4"], QFrame[frameShape="5"] { color: #45475a; }
)";

static const char* LIGHT_THEME = R"(
QMainWindow, QDialog, QWidget { background-color: #f5f5f5; color: #2d2d2d; }
QGroupBox { border: 1px solid #cccccc; border-radius: 6px; margin-top: 8px; padding-top: 8px; }
QListWidget { background: #ffffff; border: 1px solid #cccccc; border-radius: 4px; }
QListWidget::item:selected { background: #dce8ff; }
QPushButton { background: #e8e8e8; border: 1px solid #bbbbbb; border-radius: 4px; padding: 5px 10px; }
QPushButton:hover { background: #d0d8ff; }
QPushButton:disabled { color: #aaaaaa; }
QComboBox { background: #ffffff; border: 1px solid #cccccc; border-radius: 4px; padding: 3px 6px; }
QSlider::groove:horizontal { height: 4px; background: #cccccc; border-radius: 2px; }
QSlider::handle:horizontal { background: #5B8CFF; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; }
QSlider::sub-page:horizontal { background: #5B8CFF; border-radius: 2px; }
QTextEdit { background: #ffffff; border: 1px solid #cccccc; border-radius: 4px; }
QLineEdit { background: #ffffff; border: 1px solid #cccccc; border-radius: 4px; padding: 3px; }
QStatusBar { background: #ebebeb; }
)";

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    Config::load();
    ProjectManager::instance().load();

    auto engines = ToonTrad::engineList();
    auto engDisp = ToonTrad::engineDisplayNames();
    for (int i = 0; i < engines.size(); ++i)
        ui->comboEngine->addItem(engDisp[i], engines[i]);

    ui->comboDevice->addItem("Auto (détection)", "auto");
    ui->comboDevice->addItem("CPU uniquement",   "cpu");
    detectGPUs();

    /*connect(ui->sliderVRAM, &QSlider::valueChanged, this, &MainWindow::on_sliderVRAM_valueChanged);
    connect(ui->sliderRAM,  &QSlider::valueChanged, this, &MainWindow::on_sliderRAM_valueChanged);

    connect(ui->btnNewProject,    &QPushButton::clicked, this, &MainWindow::on_btnNewProject_clicked);
    connect(ui->btnRemoveProject, &QPushButton::clicked, this, &MainWindow::on_btnRemoveProject_clicked);
    connect(ui->btnOpenProject,   &QPushButton::clicked, this, &MainWindow::on_btnOpenProject_clicked);

    connect(ui->listProjects, &QListWidget::itemDoubleClicked,
            this, &MainWindow::on_listProjects_itemDoubleClicked);
    connect(ui->listProjects, &QListWidget::currentRowChanged,
            this, &MainWindow::on_listProjects_currentRowChanged);
    connect(ui->comboDevice, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::on_comboDevice_currentIndexChanged);*/

    // Détecte la RAM totale et ajuste le slider
    {
        int totalGb = 16; // valeur par défaut
#ifdef Q_OS_MAC
        int64_t mem = 0;
        size_t len = sizeof(mem);
        if (sysctlbyname("hw.memsize", &mem, &len, nullptr, 0) == 0)
            totalGb = static_cast<int>(mem / (1024LL * 1024 * 1024));
#elif defined(Q_OS_LINUX)
        struct sysinfo si;
        if (sysinfo(&si) == 0)
            totalGb = static_cast<int>(si.totalram * si.mem_unit / (1024LL * 1024 * 1024));
#endif
        ui->sliderRAM->setMaximum(qMax(totalGb, 4));
        ui->sliderRAM->setValue(qMin(4, totalGb));
        ui->lblRAMVal->setText(QString::number(ui->sliderRAM->value()) + " GB");
    }

    refreshProjectList();
    statusBar()->showMessage("Prêt");

    m_lblVersion = new QLabel(QString("v%1").arg(Config::VERSION_STR));
    m_lblVersion->setStyleSheet("color: #888; padding: 0 8px;");
    statusBar()->addPermanentWidget(m_lblVersion);

    runPipUpdate();
    checkLatestVersion();
}
void MainWindow::runPipUpdate()
{
    // Cherche requirements.txt à côté de l'exe
    QString reqPath = QCoreApplication::applicationDirPath() + "/../../../requirements.txt";
    reqPath = QDir::cleanPath(reqPath);
    if (!QFile::exists(reqPath)) return;

    QProcess* proc = new QProcess(this);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [proc](int code, QProcess::ExitStatus) {
        qDebug() << "pip update terminé, code:" << code;
        proc->deleteLater();
    });
    proc->start(Config::pythonBin, {
        "-m", "pip", "install", "-r", reqPath,
        "--upgrade", "--quiet"
    });
}

void MainWindow::checkLatestVersion()
{
    QNetworkRequest req(QUrl("https://api.github.com/repos/trotroni/toontrad/releases/latest"));
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("User-Agent", "ToonTrad");

    QNetworkReply* reply = m_network.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            m_lblVersion->setText(QString("v%1").arg(Config::VERSION_STR));
            return;
        }

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) return;

        // GitHub retourne "v2.1.0" → on enlève le "v"
        QString latest = doc.object()["tag_name"].toString();
        if (latest.startsWith("v")) latest = latest.mid(1);

        QString current = Config::VERSION_STR;
        if (latest == current)
            m_lblVersion->setText(QString("v%1 (latest)").arg(current));
        else
            m_lblVersion->setText(
                QString("v%1  ⬆ v%2 dispo").arg(current, latest));
    });
}
MainWindow::~MainWindow()
{
    Config::save();
    ProjectManager::instance().save();
    delete ui;
}


void MainWindow::detectGPUs()
{
    ui->lblGPUInfo->setText("Détection du matériel...");

    QString script = R"(
import json, sys
try:
    import torch
    gpus = []
    if torch.cuda.is_available():
        for i in range(torch.cuda.device_count()):
            p = torch.cuda.get_device_properties(i)
            gpus.append({"id": i, "name": p.name, "vram_gb": round(p.total_memory/1e9,1)})
    print(json.dumps({"cuda": torch.cuda.is_available(), "gpus": gpus}))
except Exception as e:
    print(json.dumps({"cuda": False, "gpus": [], "error": str(e)}))
)";

    QProcess* proc = new QProcess(this);
    connect(proc, QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int /*code*/, QProcess::ExitStatus) {
        QByteArray out = proc->readAllStandardOutput();
        proc->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(out);
        if (!doc.isObject()) {
            ui->lblGPUInfo->setText("⚠ PyTorch non installé — mode CPU uniquement");
            return;
        }
        QJsonObject info = doc.object();
        bool cuda = info["cuda"].toBool();

        if (!cuda) {
            ui->lblGPUInfo->setText("Pas de GPU CUDA détecté — CPU uniquement");
            return;
        }

        QString gpuInfo = "GPU(s) disponibles :\n";
        for (const QJsonValue& v : info["gpus"].toArray()) {
            QJsonObject g = v.toObject();
            QString entry = QString("  GPU %1 : %2 (%3 GB VRAM)")
                                .arg(g["id"].toInt())
                                .arg(g["name"].toString())
                                .arg(g["vram_gb"].toDouble());
            gpuInfo += entry + "\n";

            ui->comboDevice->addItem(
                QString("GPU %1 — %2 (%3 GB)")
                    .arg(g["id"].toInt())
                    .arg(g["name"].toString())
                    .arg(g["vram_gb"].toDouble()),
                QString("cuda:%1").arg(g["id"].toInt())
            );
        }
        ui->lblGPUInfo->setText(gpuInfo.trimmed());
    });

    proc->start(Config::pythonBin, {"-c", script});
}

void MainWindow::refreshProjectList()
{
    ui->listProjects->clear();
    for (const auto& p : ProjectManager::instance().projects()) {
        auto* item = new QListWidgetItem(p.name);
        item->setData(Qt::UserRole, p.rootPath);
        item->setToolTip(p.rootPath);
        ui->listProjects->addItem(item);
    }
    ui->btnOpenProject->setEnabled(false);
}

void MainWindow::on_btnNewProject_clicked()
{
    QString path = QFileDialog::getExistingDirectory(
        this, "Sélectionner le dossier du projet", QDir::homePath());
    if (path.isEmpty()) return;

    ProjectManager::instance().addProject(path);
    refreshProjectList();
    statusBar()->showMessage("Projet ajouté : " + path, 3000);
}

void MainWindow::on_btnRemoveProject_clicked()
{
    auto* item = ui->listProjects->currentItem();
    if (!item) return;

    QString path = item->data(Qt::UserRole).toString();
    auto btn = QMessageBox::question(this, "Supprimer",
        "Retirer le projet de la liste ?\n(les fichiers ne seront pas supprimés)");
    if (btn != QMessageBox::Yes) return;

    ProjectManager::instance().removeProject(path);
    refreshProjectList();
}

void MainWindow::on_listProjects_currentRowChanged(int row)
{
    ui->btnOpenProject->setEnabled(row >= 0);
}

void MainWindow::on_listProjects_itemDoubleClicked()
{
    openSelectedProject();
}

void MainWindow::on_btnOpenProject_clicked()
{
    openSelectedProject();
}

void MainWindow::openSelectedProject()
{
    auto* item = ui->listProjects->currentItem();
    if (!item) return;

    QString path = item->data(Qt::UserRole).toString();

    if (m_openWindows.contains(path) && !m_openWindows[path].isNull()) {
        m_openWindows[path]->raise();
        m_openWindows[path]->activateWindow();
        return;
    }

    auto& projects = ProjectManager::instance().projects();
    for (auto& p : projects) {
        if (p.rootPath != path) continue;

        OCRwindow* w = new OCRwindow(&p, currentConfig(), nullptr);
        w->setAttribute(Qt::WA_DeleteOnClose);

        connect(w, &QObject::destroyed, this, [this, path]() {
            m_openWindows.remove(path);
        });

        m_openWindows[path] = w;
        w->show();
        return;
    }
}


OCRConfig MainWindow::currentConfig() const
{
    OCRConfig c = m_config;
    c.engine          = ui->comboEngine->currentData().toString();
    c.device          = ui->comboDevice->currentData().toString();
    c.gpuMemFraction  = ui->sliderVRAM->value() / 100.0;
    // Convertit GB → fraction (Python attend une fraction de la RAM totale)
    {
        double totalGb = qMax(ui->sliderRAM->maximum(), 1);
        c.ramFraction  = ui->sliderRAM->value() / totalGb;
    }

    if (c.device.startsWith("cuda:"))
        c.gpuId = c.device.mid(5).toInt();
    return c;
}

void MainWindow::on_sliderVRAM_valueChanged(int value)
{
    ui->lblVRAMVal->setText(QString::number(value) + "%");
}

void MainWindow::on_sliderRAM_valueChanged(int value)
{
    ui->lblRAMVal->setText(QString::number(value) + " GB");
}

void MainWindow::on_comboDevice_currentIndexChanged(int /*index*/)
{
    QString dev = ui->comboDevice->currentData().toString();
    bool gpuSelected = dev.startsWith("cuda:");
    ui->sliderVRAM->setEnabled(gpuSelected);
    ui->lblVRAM->setEnabled(gpuSelected);
    ui->lblVRAMVal->setEnabled(gpuSelected);
    ui->sliderRAM->setEnabled(!gpuSelected || dev == "cpu");
}

void MainWindow::on_btnSettings_clicked()
{
    SettingsWindow dlg(currentConfig(), this);
    if (dlg.exec() == QDialog::Accepted)
        m_config = dlg.config();
}

void MainWindow::applyTheme()
{
    qApp->setStyleSheet("");
}