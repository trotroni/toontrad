#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "../../config.h"
#include "../../core/BubbleDetector.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QDebug>

// ─────────────────────────────────────────────────────────────────────────────
//  Constructeur
// ─────────────────────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    Config::load();

    // ── ImageCanvas dans le scroll gauche ────────────────────────────────────
    auto* canvas = new ImageCanvas(this);
    auto* scrollLayout = new QVBoxLayout(ui->canvasContainer);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->addWidget(canvas);
    ui->canvasContainer->setLayout(scrollLayout);

    // ── Layout scroll bulles ─────────────────────────────────────────────────
    m_scrollWidget = ui->bubblesContent;
    m_scrollLayout = qobject_cast<QVBoxLayout*>(m_scrollWidget->layout());

    // ── Connexions boutons ───────────────────────────────────────────────────
    connect(ui->btnPrev,         &QPushButton::clicked, this, &MainWindow::onBtnPrev);
    connect(ui->btnNext,         &QPushButton::clicked, this, &MainWindow::onBtnNext);
    connect(ui->btnSave,         &QPushButton::clicked, this, &MainWindow::onBtnSave);
    connect(ui->btnAdd,          &QPushButton::clicked, this, &MainWindow::onBtnAdd);
    connect(ui->btnOpenFolder,   &QPushButton::clicked, this, &MainWindow::onBtnOpenFolder);
    connect(ui->btnReloadConfig, &QPushButton::clicked, this, &MainWindow::onBtnReloadConfig);

    // ── Connexions canvas ────────────────────────────────────────────────────
    connect(canvas, &ImageCanvas::bubbleDeleteRequested,
            this,   &MainWindow::onBubbleDeleteRequested);
    connect(canvas, &ImageCanvas::bubbleAddRequested,
            this,   &MainWindow::onBubbleAddRequested);

    // ── Chargement initial ───────────────────────────────────────────────────
    if (!Config::folderPath.isEmpty())
        loadImages(Config::folderPath);
    else
        log("Ouvrez un dossier pour commencer.");

    updateNavButtons();
}

MainWindow::~MainWindow()
{
    Config::save();
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

QString MainWindow::currentImagePath() const
{
    if (m_imageList.isEmpty()) return {};
    if (m_currentIndex < 0 || m_currentIndex >= m_imageList.size()) return {};
    return m_imageList[m_currentIndex];
}

void MainWindow::log(const QString& msg)
{
    ui->console->append(msg);
}

void MainWindow::recalculateIds()
{
    for (int i = 0; i < (int)m_bubbles.size(); ++i)
        m_bubbles[i].id = i + 1;
}

void MainWindow::syncBubbles()
{
    for (auto* w : m_bubbleWidgets) {
        for (auto& b : m_bubbles) {
            if (b.id == w->bubbleId()) {
                w->syncToBubble(b);
                break;
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Chargement images
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::loadImages(const QString& folderPath)
{
    QDir dir(folderPath);
    QStringList filters = {"*.jpg", "*.jpeg", "*.png", "*.bmp", "*.webp"};
    QStringList files   = dir.entryList(filters, QDir::Files, QDir::Name);

    if (files.isEmpty()) {
        QMessageBox::warning(this, "Dossier vide", "Aucune image trouvée dans ce dossier.");
        return;
    }

    m_imageList.clear();
    for (const QString& f : files)
        m_imageList << dir.absoluteFilePath(f);

    m_currentIndex = 0;
    Config::folderPath = folderPath;
    Config::save();

    log(QString("%1 image(s) chargée(s) depuis %2").arg(files.size()).arg(folderPath));
    loadCurrentImage();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Chargement image courante — identique à load_current_image() Python
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::loadCurrentImage()
{
    QString path = currentImagePath();
    if (path.isEmpty()) return;

    ui->lblImageName->setText(QFileInfo(path).fileName());
    statusBar()->showMessage("Détection en cours…");
    qApp->processEvents();

    // Vérification Python
    QString pyErr;
    if (!BubbleDetector::checkAvailable(&pyErr)) {
        QMessageBox::critical(this, "Python introuvable", pyErr);
        statusBar()->showMessage("Erreur Python");
        return;
    }

    // Détection via QProcess → detect.py
    BubbleDetector detector(this);
    connect(&detector, &BubbleDetector::errorOccurred, this, [this](const QString& msg) {
        QMessageBox::critical(this, "Erreur détection", msg);
        log("Erreur : " + msg);
    });

    m_bubbles = detector.run(path);
    log(QString("%1 bulle(s) détectée(s) — %2")
            .arg(m_bubbles.size())
            .arg(QFileInfo(path).fileName()));

    // Mise à jour canvas
    auto* canvas = findChild<ImageCanvas*>();
    if (canvas) {
        canvas->setImage(QPixmap(path));
        canvas->setBubbles(m_bubbles);
    }

    buildRightPanel();
    updateNavButtons();
    statusBar()->showMessage(QString("Page %1 / %2")
                                 .arg(m_currentIndex + 1)
                                 .arg(m_imageList.size()));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Panneau droit — identique à build_right_panel() Python
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::buildRightPanel()
{
    // Supprime les anciens widgets
    for (auto* w : m_bubbleWidgets)
        w->deleteLater();
    m_bubbleWidgets.clear();

    // Supprime tous les items sauf le spacer final
    while (m_scrollLayout->count() > 1) {
        QLayoutItem* item = m_scrollLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    // Crée un BubbleWidget par bulle
    for (const auto& b : m_bubbles) {
        auto* w = new BubbleWidget(b, m_scrollWidget);
        connect(w, &BubbleWidget::deleteRequested,
                this, &MainWindow::onBubbleDeleteRequested);
        connect(w, &BubbleWidget::textChanged,
                this, &MainWindow::onTextChanged);

        // Insère avant le spacer
        m_scrollLayout->insertWidget(m_scrollLayout->count() - 1, w);
        m_bubbleWidgets.append(w);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Navigation
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::updateNavButtons()
{
    ui->btnPrev->setEnabled(m_currentIndex > 0);
    ui->btnNext->setEnabled(m_currentIndex < m_imageList.size() - 1);
}

void MainWindow::onBtnPrev()
{
    if (m_currentIndex > 0) {
        m_currentIndex--;
        loadCurrentImage();
    }
}

void MainWindow::onBtnNext()
{
    if (m_currentIndex < m_imageList.size() - 1) {
        m_currentIndex++;
        loadCurrentImage();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Actions
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onBtnOpenFolder()
{
    QString path = QFileDialog::getExistingDirectory(
        this, "Sélectionner le dossier d'images", QDir::homePath());
    if (!path.isEmpty())
        loadImages(path);
}

void MainWindow::onBtnSave()
{
    syncBubbles();

    QString imageName = QFileInfo(currentImagePath()).fileName();
    Exporter exp;
    QString saved = exp.exportTxt(m_bubbles, imageName, Config::outputFolder);

    if (!saved.isEmpty()) {
        log("Sauvegardé : " + saved);
        statusBar()->showMessage("Sauvegardé → " + saved, 3000);
    } else {
        QMessageBox::warning(this, "Erreur", "Échec de la sauvegarde.");
    }
}

void MainWindow::onBtnAdd()
{
    auto* canvas = findChild<ImageCanvas*>();
    if (canvas) {
        canvas->setAddMode(true);
        log("Mode ajout activé : glisser pour sélectionner une bulle");
    }
}

void MainWindow::onBtnReloadConfig()
{
    Config::load();
    log("Configuration rechargée");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Événements canvas
// ─────────────────────────────────────────────────────────────────────────────

void MainWindow::onBubbleDeleteRequested(int id)
{
    m_bubbles.erase(
        std::remove_if(m_bubbles.begin(), m_bubbles.end(),
                       [id](const Bubble& b) { return b.id == id; }),
        m_bubbles.end());

    recalculateIds();
    buildRightPanel();

    auto* canvas = findChild<ImageCanvas*>();
    if (canvas) canvas->setBubbles(m_bubbles);

    log(QString("Bulle %1 supprimée").arg(id));
}

void MainWindow::onBubbleAddRequested(QRect rect)
{
    int nextId = m_bubbles.empty() ? 1 : m_bubbles.back().id + 1;
    Bubble newBubble(nextId, rect);
    m_bubbles.push_back(newBubble);

    recalculateIds();
    buildRightPanel();

    auto* canvas = findChild<ImageCanvas*>();
    if (canvas) canvas->setBubbles(m_bubbles);

    log(QString("Bulle %1 ajoutée à %2,%3,%4,%5")
            .arg(nextId)
            .arg(rect.x()).arg(rect.y())
            .arg(rect.width()).arg(rect.height()));
}

void MainWindow::onTextChanged()
{
    // Sync live vers m_bubbles puis refresh canvas (label texte sur image)
    syncBubbles();
    auto* canvas = findChild<ImageCanvas*>();
    if (canvas) canvas->setBubbles(m_bubbles);
}