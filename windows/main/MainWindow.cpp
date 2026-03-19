#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "../../config.h"
#include "../../core/models.h"
#include "../../core/ProjectManager.h"
#include "../settings/SettingsWindow.h"
#include "../project/NewProjectDialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QScreen>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDebug>

#ifdef Q_OS_MAC
#include <sys/sysctl.h>
#endif
#ifdef Q_OS_LINUX
#include <sys/sysinfo.h>
#endif

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    Config::load();
    ProjectManager::instance().load();

    // Restaure la config OCR sauvegardée
    m_config = Config::ocrConfig;

    // Restaure les widgets depuis m_config
    {
        auto engines = ToonTrad::engineList();
        int ei = engines.indexOf(m_config.engine);
        if (ei >= 0) ui->comboEngine->setCurrentIndex(ei);
    }
    ui->sliderVRAM->setValue(static_cast<int>(m_config.gpuMemFraction * 100));
    ui->lblVRAMVal->setText(QString::number(ui->sliderVRAM->value()) + "%");

    // Moteurs
    auto engines = ToonTrad::engineList();
    auto engDisp = ToonTrad::engineDisplayNames();
    for (int i = 0; i < engines.size(); ++i)
        ui->comboEngine->addItem(engDisp[i], engines[i]);

    // Devices
    ui->comboDevice->addItem("Auto (détection)", "auto");
    ui->comboDevice->addItem("CPU uniquement",   "cpu");
    detectGPUs();

    // RAM slider
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

    // Version
    m_lblVersion = new QLabel(QString("v%1").arg(Config::VERSION_STR));
    m_lblVersion->setStyleSheet("color: #888; padding: 0 8px;");
    statusBar()->addPermanentWidget(m_lblVersion);

    refreshProjectList();
    statusBar()->showMessage("Prêt");

    runPipUpdate();
    checkLatestVersion();
}

MainWindow::~MainWindow()
{
    Config::save();
    ProjectManager::instance().save();
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Projets
// ─────────────────────────────────────────────────────────────────────────────

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
    NewProjectDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted) return;

    QString path = dlg.projectPath();
    ProjectManager::instance().addProject(path);
    refreshProjectList();
    statusBar()->showMessage("Projet créé : " + path, 4000);
}

void MainWindow::on_btnRemoveProject_clicked()
{
    auto* item = ui->listProjects->currentItem();
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    if (QMessageBox::question(this, "Supprimer",
            "Retirer le projet de la liste ?\n(les fichiers ne seront pas supprimés)")
        != QMessageBox::Yes) return;
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

    // Si déjà ouvert, ramener au premier plan
    if (m_openWindows.contains(path)) {
        auto& wins = m_openWindows[path];
        if (!wins.imageWin.isNull()) { wins.imageWin->raise(); wins.imageWin->activateWindow(); }
        if (!wins.textWin.isNull())  { wins.textWin->raise();  wins.textWin->activateWindow(); }
        return;
    }

    // Trouve le projet
    for (auto& p : ProjectManager::instance().projects()) {
        if (p.rootPath != path) continue;

        auto* imgWin  = new ImageWindow(&p, currentConfig(), nullptr);
        auto* textWin = new TextWindow(nullptr);

        imgWin->setAttribute(Qt::WA_DeleteOnClose);
        textWin->setAttribute(Qt::WA_DeleteOnClose);

        // ── Synchronisation bidirectionnelle ──────────────────────────────────

        // ImageWindow → TextWindow : changement de blocs
        connect(imgWin, &ImageWindow::blocksChanged,
                textWin, &TextWindow::setBlocks);

        // ImageWindow → TextWindow : clic bulle sur image
        connect(imgWin, &ImageWindow::blockSelected,
                textWin, &TextWindow::scrollToBlock);

        // TextWindow → ImageWindow : modification d'un champ
        connect(textWin, &TextWindow::blockUpdated,
                imgWin,  &ImageWindow::onBlockUpdated);

        // TextWindow → ImageWindow : clic sur un panneau texte
        connect(textWin, &TextWindow::blockSelected,
                imgWin,  &ImageWindow::highlightBlock);

        // TextWindow → ImageWindow : réordonnancement par drag
        connect(textWin, &TextWindow::blocksReordered,
                imgWin, [imgWin](const QList<int>& newOrder) {
            imgWin->reorderBlocks(newOrder);
        });

        // ── Positionnement côte à côte adapté à l'écran ─────────────────────
        QScreen* screen = QGuiApplication::primaryScreen();
        QRect    avail  = screen->availableGeometry();

        // TextWindow : largeur fixe 420px
        int textW  = 420;
        int imgW   = avail.width() - textW - 10;
        int height = avail.height();

        imgWin->setGeometry(avail.x(), avail.y(), imgW, height);
        textWin->setGeometry(avail.x() + imgW + 5, avail.y(), textW, height);

        imgWin->show();
        textWin->show();

        m_openWindows[path] = {imgWin, textWin};

        // Nettoyage à la fermeture
        connect(imgWin, &QObject::destroyed, this, [this, path]() {
            m_openWindows.remove(path);
        });

        return;
    }
}

OCRConfig MainWindow::currentConfig() const
{
    OCRConfig c = m_config;
    c.engine         = ui->comboEngine->currentData().toString();
    c.device         = ui->comboDevice->currentData().toString();
    c.gpuMemFraction = ui->sliderVRAM->value() / 100.0;
    int totalGb      = qMax(ui->sliderRAM->maximum(), 1);
    c.ramGb          = ui->sliderRAM->value();
    c.ramFraction    = c.ramGb / static_cast<double>(totalGb);
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
}

void MainWindow::on_comboEngine_currentIndexChanged(int)
{
    // Réservé pour adaptations futures
}

void MainWindow::on_btnSettings_clicked()
{
    SettingsWindow dlg(currentConfig(), this);
    if (dlg.exec() == QDialog::Accepted) {
        m_config = dlg.config();
        Config::saveOCR(m_config);
    }
}

void MainWindow::on_btnReloadProject_clicked()
{
    auto* item = ui->listProjects->currentItem();
    if (!item) {
        statusBar()->showMessage("Aucun projet sélectionné.", 3000);
        return;
    }

    QString path = item->data(Qt::UserRole).toString();
    for (auto& p : ProjectManager::instance().projects()) {
        if (p.rootPath != path) continue;

        p.scanImages();
        p.save();

        statusBar()->showMessage(
            QString("Projet rechargé : %1 image(s) dans raw/")
                .arg(p.pages.size()), 4000);
        return;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  GPU Detection
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
            ui->lblGPUInfo->setText("⚠ PyTorch non installé — CPU uniquement");
            return;
        }
        QJsonObject info = doc.object();
        if (!info["cuda"].toBool()) {
            ui->lblGPUInfo->setText("Pas de GPU CUDA — CPU uniquement");
            return;
        }
        QString txt;
        for (const QJsonValue& v : info["gpus"].toArray()) {
            QJsonObject g = v.toObject();
            txt += QString("GPU %1 : %2 (%3 GB VRAM)\n")
                       .arg(g["id"].toInt())
                       .arg(g["name"].toString())
                       .arg(g["vram_gb"].toDouble());
            ui->comboDevice->addItem(
                QString("GPU %1 — %2 (%3 GB)")
                    .arg(g["id"].toInt()).arg(g["name"].toString()).arg(g["vram_gb"].toDouble()),
                QString("cuda:%1").arg(g["id"].toInt()));
        }
        ui->lblGPUInfo->setText(txt.trimmed());
    });
    proc->start(Config::pythonBin, {"-c", script});
}

// ─────────────────────────────────────────────────────────────────────────────
//  Version + pip update
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::checkLatestVersion()
{
    QNetworkRequest req(QUrl("https://api.github.com/repos/trotroni/toontrad/releases/latest"));
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setRawHeader("User-Agent", "ToonTrad");

    QNetworkReply* reply = m_network.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) return;
        QString latest = doc.object()["tag_name"].toString();
        if (latest.startsWith("v")) latest = latest.mid(1);
        QString current = Config::VERSION_STR;
        if (latest == current)
            m_lblVersion->setText(QString("v%1 (latest)").arg(current));
        else
            m_lblVersion->setText(QString("v%1  ⬆ v%2 dispo").arg(current, latest));
    });
}

void MainWindow::runPipUpdate()
{
    QString reqPath = QDir::cleanPath(
        QCoreApplication::applicationDirPath() + "/../../../requirements.txt");
    if (!QFile::exists(reqPath)) return;

    QProcess* proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [proc](int code, QProcess::ExitStatus) {
        qDebug() << "pip update terminé, code:" << code;
        proc->deleteLater();
    });
    proc->start(Config::pythonBin,
        {"-m", "pip", "install", "-r", reqPath, "--upgrade", "--quiet"});
}
