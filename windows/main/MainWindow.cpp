#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "../../config.h"
#include "../../core/models.h"
#include "../../core/ProjectManager.h"
#include "../image/ImageWindow.h"
#include "../text/TextWindow.h"
#include "../settings/SettingsWindow.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFileOpenEvent>
#include <QMessageBox>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QNetworkRequest>

#ifdef Q_OS_MAC
#  include <sys/sysctl.h>
#endif
#ifdef Q_OS_LINUX
#  include <sys/sysinfo.h>
#endif


// ─────────────────────────────────────────────────────────────────────────────
//  Constructeur
// ─────────────────────────────────────────────────────────────────────────────

MainWindow::MainWindow(const QString& openPath, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    qDebug() << "[MainWindow] open";
    ui->setupUi(this);

    // Moteurs OCR
    auto engines = ToonTrad::engineList();
    auto engDisp = ToonTrad::engineDisplayNames();
    for (int i = 0; i < engines.size(); ++i)
        ui->comboEngine->addItem(engDisp[i], engines[i]);

    // Devices
    ui->comboDevice->addItem("Auto (détection)", "auto");
    ui->comboDevice->addItem("CPU uniquement",   "cpu");
    detectGPUs();

    // Slider RAM : détecte la mémoire totale
    {
        int totalGb = 16;
#ifdef Q_OS_MAC
        int64_t mem = 0; size_t len = sizeof(mem);
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

    // Ouvre le projet passé en argument (double-clic OS)
    if (!openPath.isEmpty())
        openProjectFromPath(openPath);
}

MainWindow::~MainWindow()
{
    Config::save();
    ProjectManager::instance().save();
    delete ui;
    qDebug() << "[MainWindow] delete ui";
}


// ─────────────────────────────────────────────────────────────────────────────
//  Gestion QFileOpenEvent (macOS bundle — double-clic Finder)
// ─────────────────────────────────────────────────────────────────────────────

bool MainWindow::event(QEvent* e)
{
    if (e->type() == QEvent::FileOpen) {
        auto* foe = static_cast<QFileOpenEvent*>(e);
        openProjectFromPath(foe->file());
        return true;
    }
    return QMainWindow::event(e);
}


// ─────────────────────────────────────────────────────────────────────────────
//  Ouverture depuis un fichier .ttproject
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::openProjectFromPath(const QString& filePath)
{
    if (filePath.isEmpty()) return;

    QFileInfo fi(filePath);
    if (!fi.exists()) {
        QMessageBox::warning(this, "Fichier introuvable",
            "Impossible d'ouvrir : " + filePath);
        return;
    }

    QString rootPath = fi.isDir() ? filePath : fi.absolutePath();

    qDebug() << "openProjectFromPath → rootPath =" << rootPath;

    // Ajoute le projet s'il n'est pas déjà connu
    ProjectManager::instance().addProject(rootPath);
    refreshProjectList();

    // Sélectionne-le dans la liste et ouvre la fenêtre
    openProject(rootPath);
}


// ─────────────────────────────────────────────────────────────────────────────
//  Gestion des projets
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::refreshProjectList()
{
    qDebug() << "[MainWindow] refreshProjectList";
    ui->listProjects->clear();
    for (const auto& p : ProjectManager::instance().projects()) {
        auto* item = new QListWidgetItem(p.name);
        item->setData(Qt::UserRole, p.rootPath);
        item->setToolTip(p.rootPath);
        ui->listProjects->addItem(item);
    }
    ui->btnOpenProject->setEnabled(false);
}

void MainWindow::openProject(const QString& rootPath)
{
    qDebug() << "[MainWindow] openProject";
    if (m_openWindows.contains(rootPath) && !m_openWindows[rootPath].isNull()) {
        m_openWindows[rootPath]->raise();
        m_openWindows[rootPath]->activateWindow();
        return;
    }

    auto& projects = ProjectManager::instance().projects();
    for (auto& p : projects) {
        if (p.rootPath != rootPath) continue;

        ImageWindow* imageWin = new ImageWindow(&p, currentConfig(), nullptr);
        imageWin->setAttribute(Qt::WA_DeleteOnClose);

        TextWindow* textWin = new TextWindow(nullptr);
        textWin->setAttribute(Qt::WA_DeleteOnClose);

        // ── ImageWindow → TextWindow ──────────────────────────────────────
        connect(imageWin, &ImageWindow::blocksChanged,
                textWin,  &TextWindow::setBlocks);
        connect(imageWin, &ImageWindow::blockSelected,
                textWin,  &TextWindow::scrollToBlock);

        // ── TextWindow → ImageWindow ──────────────────────────────────────
        connect(textWin,  &TextWindow::blockUpdated,
                imageWin, &ImageWindow::onBlockUpdated);
        connect(textWin,  &TextWindow::blockSelected,
                imageWin, &ImageWindow::highlightBlock);
        connect(textWin,  &TextWindow::blocksReordered,
                imageWin, &ImageWindow::reorderBlocks);

        // ── Fermeture liée : l'un ferme l'autre ──────────────────────────
        connect(imageWin, &QObject::destroyed,
                textWin,  &QWidget::close);
        connect(textWin,  &QObject::destroyed,
                imageWin, &QWidget::close);

        // ── Nettoyage de la map ───────────────────────────────────────────
        connect(imageWin, &QObject::destroyed, this, [this, rootPath]() {
            m_openWindows.remove(rootPath);
        });

        m_openWindows[rootPath] = imageWin;

        imageWin->show();
        textWin->show();
        return;
    }
}

void MainWindow::openSelectedProject()
{
    qDebug() << "[MainWindow] openSelectedProject";
    auto* item = ui->listProjects->currentItem();
    if (!item) return;
    openProject(item->data(Qt::UserRole).toString());
}

void MainWindow::on_btnNewProject_clicked()
{
    qDebug() << "[MainWindow] on_btnNewProject_clicked";

    NewProjectDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    QString projectPath = dlg.projectPath();
    if (projectPath.isEmpty()) return;

    ProjectManager::instance().addProject(projectPath);
    refreshProjectList();
    statusBar()->showMessage("Projet créé : " + projectPath, 3000);
}

void MainWindow::on_btnRemoveProject_clicked()
{
    qDebug() << "[MainWindow] on_btnRemoveProject_clicked";
    auto* item = ui->listProjects->currentItem();
    if (!item) return;

    QString path = item->data(Qt::UserRole).toString();
    if (QMessageBox::question(this, "Supprimer",
            "Retirer le projet de la liste ?\n(les fichiers ne seront pas supprimés)")
        != QMessageBox::Yes) return;

    ProjectManager::instance().removeProject(path);
    refreshProjectList();
}

void MainWindow::on_btnReloadProject_clicked()
{
    qDebug() << "[MainWindow] on_btnReloadProject_clicked";
    auto* item = ui->listProjects->currentItem();
    if (!item) return;

    QString path = item->data(Qt::UserRole).toString();
    for (auto& p : ProjectManager::instance().projects()) {
        if (p.rootPath != path) continue;
        p.scanImages();
        p.save();
        statusBar()->showMessage("Projet rechargé : " + path, 3000);
        break;
    }
}

void MainWindow::on_listProjects_currentRowChanged(int row)
{
    qDebug() << "[MainWindow] on_listProjects_currentRowChanged";
    ui->btnOpenProject->setEnabled(row >= 0);
}

void MainWindow::on_listProjects_itemDoubleClicked()
{
    qDebug() << "[MainWindow] on_listProjects_itemDoubleClicked";
    openSelectedProject();
}

void MainWindow::on_btnOpenProject_clicked()
{
    qDebug() << "[MainWindow] on_btnOpenProject_clicked";
    openSelectedProject();
}


// ─────────────────────────────────────────────────────────────────────────────
//  GPU / Config
// ─────────────────────────────────────────────────────────────────────────────

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
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int, QProcess::ExitStatus) {
        QByteArray out = proc->readAllStandardOutput();
        proc->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(out);
        if (!doc.isObject()) {
            ui->lblGPUInfo->setText("⚠ PyTorch non installé — mode CPU uniquement");
            return;
        }
        QJsonObject info = doc.object();
        if (!info["cuda"].toBool()) {
            ui->lblGPUInfo->setText("Pas de GPU CUDA détecté — CPU uniquement");
            return;
        }

        QString gpuInfo;
        for (const QJsonValue& v : info["gpus"].toArray()) {
            QJsonObject g = v.toObject();
            gpuInfo += QString("  GPU %1 : %2 (%3 GB VRAM)\n")
                           .arg(g["id"].toInt())
                           .arg(g["name"].toString())
                           .arg(g["vram_gb"].toDouble());
            ui->comboDevice->addItem(
                QString("GPU %1 — %2 (%3 GB)")
                    .arg(g["id"].toInt()).arg(g["name"].toString())
                    .arg(g["vram_gb"].toDouble()),
                QString("cuda:%1").arg(g["id"].toInt()));
        }
        ui->lblGPUInfo->setText(gpuInfo.trimmed());
    });
    proc->start(Config::pythonBin, {"-c", script});
}

OCRConfig MainWindow::currentConfig() const
{
    OCRConfig c = m_config;
    c.engine         = ui->comboEngine->currentData().toString();
    c.device         = ui->comboDevice->currentData().toString();
    c.gpuMemFraction = ui->sliderVRAM->value() / 100.0;
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

void MainWindow::on_comboDevice_currentIndexChanged(int)
{
    QString dev = ui->comboDevice->currentData().toString();
    bool gpu = dev.startsWith("cuda:");
    ui->sliderVRAM->setEnabled(gpu);
    ui->lblVRAM->setEnabled(gpu);
    ui->lblVRAMVal->setEnabled(gpu);
    ui->sliderRAM->setEnabled(!gpu);
}

void MainWindow::on_btnSettings_clicked()
{
    qDebug() << "[MainWindow] on_btnSettings_clicked";
    SettingsWindow dlg(currentConfig(), this);
    if (dlg.exec() == QDialog::Accepted)
        m_config = dlg.config();
}


// ─────────────────────────────────────────────────────────────────────────────
//  Utilitaires
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::runPipUpdate()
{
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
    proc->start(Config::pythonBin,
        {"-m", "pip", "install", "-r", reqPath, "--upgrade", "--quiet"});
}

void MainWindow::checkLatestVersion()
{
    QNetworkRequest req(
        QUrl("https://api.github.com/repos/trotroni/toontrad/releases/latest"));
    req.setRawHeader("Accept",     "application/vnd.github+json");
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
