#include "OCRwindow.h"
#include "ui_OCRwindow.h"
#include "../../core/OCRManager.h"
#include "../../core/Exporter.h"
#include "../../gui/canvas/ImageCanvas.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QPushButton>
#include <QProgressDialog>
#include <QApplication>
#include <QDebug>

// ─────────────────────────────────────────────────────────────────────────────
//  Constructeur
// ─────────────────────────────────────────────────────────────────────────────

OCRwindow::OCRwindow(Project* project, const OCRConfig& config, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::OCRwindow)
    , m_project(project)
    , m_config(config)
{
    ui->setupUi(this);
    setWindowTitle("ToonTrad — " + project->name);

    // Remplace le QGraphicsView du .ui par notre ImageCanvas custom
    auto* oldCanvas = ui->imageCanvas;
    auto* canvas    = new ImageCanvas(ui->imageFrame);
    oldCanvas->parentWidget()->layout()->replaceWidget(oldCanvas, canvas);
    oldCanvas->deleteLater();
    ui->imageCanvas = canvas;  // réassigne le pointeur (cast ci-dessous)

    // Clic droit sur une zone → suppression
    connect(canvas, &ImageCanvas::blockRightClicked,
            this, &OCRwindow::onBlockDeleted);

    // Connexions boutons
    connect(ui->btnPrev,        &QPushButton::clicked, this, &OCRwindow::on_btnPrev_clicked);
    connect(ui->btnNext,        &QPushButton::clicked, this, &OCRwindow::on_btnNext_clicked);
    connect(ui->btnRunOCR,      &QPushButton::clicked, this, &OCRwindow::on_btnRunOCR_clicked);
    connect(ui->btnSave,        &QPushButton::clicked, this, &OCRwindow::on_btnSave_clicked);
    connect(ui->btnExportJSON,  &QPushButton::clicked, this, &OCRwindow::on_btnExportJSON_clicked);
    connect(ui->btnExportPNG,   &QPushButton::clicked, this, &OCRwindow::on_btnExportPNG_clicked);
    connect(ui->btnExportTXT,   &QPushButton::clicked, this, &OCRwindow::on_btnExportTXT_clicked);

    if (!project->pages.isEmpty())
        loadPage(0);
    else
        ui->lblStatus->setText("Aucune image dans ce projet.");
}

OCRwindow::~OCRwindow()
{
    delete ui;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

ImagePage* OCRwindow::currentPage()
{
    if (m_project->pages.isEmpty()) return nullptr;
    if (m_currentPageIndex < 0 || m_currentPageIndex >= m_project->pages.size())
        return nullptr;
    return &m_project->pages[m_currentPageIndex];
}

QString OCRwindow::currentImagePath()
{
    ImagePage* p = currentPage();
    return p ? m_project->absolutePath(*p) : QString();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Navigation
// ─────────────────────────────────────────────────────────────────────────────

void OCRwindow::loadPage(int index)
{
    // Sauvegarde silencieuse uniquement si la page courante a des blocs
    ImagePage* cur = currentPage();
    if (cur && cur->ocrDone)
        m_project->save();

    m_currentPageIndex = index;
    clearBlockWidgets();
    displayImage();
    displayBlocks();
    updateNavButtons();
}

void OCRwindow::displayImage()
{
    auto* canvas = qobject_cast<ImageCanvas*>(ui->imageCanvas);
    if (!canvas) return;

    QString path = currentImagePath();
    if (path.isEmpty()) { canvas->setImage({}); return; }

    QImage img(path);
    if (img.isNull()) {
        ui->lblStatus->setText("Impossible de charger : " + path);
        return;
    }
    canvas->setImage(img);
}

void OCRwindow::displayBlocks()
{
    auto* canvas = qobject_cast<ImageCanvas*>(ui->imageCanvas);
    ImagePage* page = currentPage();
    if (!page) return;

    clearBlockWidgets();
    if (canvas) canvas->setBlocks(page->blocks);

    for (const auto& b : page->blocks)
        addBlockWidget(b);
}

void OCRwindow::updateNavButtons()
{
    int total = m_project->pages.size();
    ui->btnPrev->setEnabled(m_currentPageIndex > 0);
    ui->btnNext->setEnabled(m_currentPageIndex < total - 1);
    ui->lblPage->setText(
        QString("Page %1 / %2  —  %3")
            .arg(m_currentPageIndex + 1)
            .arg(total)
            .arg(QFileInfo(currentImagePath()).fileName())
    );
}

void OCRwindow::on_btnPrev_clicked()
{
    if (m_currentPageIndex > 0)
        loadPage(m_currentPageIndex - 1);
}

void OCRwindow::on_btnNext_clicked()
{
    if (m_currentPageIndex < m_project->pages.size() - 1)
        loadPage(m_currentPageIndex + 1);
}

// ─────────────────────────────────────────────────────────────────────────────
//  OCR
// ─────────────────────────────────────────────────────────────────────────────

void OCRwindow::on_btnRunOCR_clicked()
{
    ImagePage* page = currentPage();
    if (!page) return;

    QString imgPath = currentImagePath();
    if (!QFile::exists(imgPath)) {
        QMessageBox::warning(this, "Erreur", "Image introuvable : " + imgPath);
        return;
    }

    // Vérification Python
    QString pyErr;
    if (!OCRManager::checkPythonAvailable(&pyErr)) {
        QMessageBox::critical(this, "Python introuvable",
            pyErr + "\n\nVérifiez les chemins dans Fichier → Paramètres OCR.");
        return;
    }

    ui->btnRunOCR->setEnabled(false);
    ui->lblStatus->setText("OCR en cours…");
    qApp->processEvents();

    OCRManager ocr(this);
    connect(&ocr, &OCRManager::errorOccurred, this, [this](const QString& msg) {
        QMessageBox::critical(this, "Erreur OCR", msg);
        ui->lblStatus->setText("Erreur OCR");
    });

    auto blocks = ocr.runOCR(imgPath, m_config);

    page->blocks = std::move(blocks);
    page->ocrDone = !page->blocks.empty();

    displayBlocks();
    ui->lblStatus->setText(
        QString("OCR terminé : %1 bloc(s) détecté(s)").arg(page->blocks.size()));
    ui->btnRunOCR->setEnabled(true);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Blocs UI
// ─────────────────────────────────────────────────────────────────────────────

void OCRwindow::addBlockWidget(const TextBlock& block)
{
    QFrame* frame = new QFrame();
    frame->setFrameShape(QFrame::Box);

    QVBoxLayout* vl = new QVBoxLayout(frame);
    vl->setContentsMargins(6,6,6,6);
    vl->setSpacing(4);

    // En-tête
    QHBoxLayout* hl = new QHBoxLayout();
    QLabel* idLbl   = new QLabel(QString("Bulle #%1").arg(block.id));
    idLbl->setStyleSheet("font-weight: bold;");

    QLabel* confLbl = new QLabel(QString("conf: %1%")
                                     .arg(static_cast<int>(block.confidence*100)));
    confLbl->setStyleSheet(
        block.confidence >= 0.8 ? "color: #0a0;" :
        block.confidence >= 0.5 ? "color: #a60;" : "color: #c00;");

    QPushButton* btnDel = new QPushButton("✕");
    btnDel->setFixedSize(22,22);
    btnDel->setToolTip("Supprimer ce bloc");

    int bid = block.id;
    connect(btnDel, &QPushButton::clicked, this, [this, bid]() {
        onBlockDeleted(bid);
    });

    hl->addWidget(idLbl);
    hl->addWidget(confLbl);
    hl->addStretch();
    hl->addWidget(btnDel);

    // Texte original
    QLabel* lblOrig = new QLabel("Original :");
    lblOrig->setStyleSheet("font-size: 11px; color: #888;");
    QTextEdit* origEdit = new QTextEdit();
    origEdit->setText(block.originalText);
    origEdit->setMaximumHeight(65);

    // Texte traduit
    QLabel* lblTrad = new QLabel("Traduction :");
    lblTrad->setStyleSheet("font-size: 11px; color: #888;");
    QTextEdit* tradEdit = new QTextEdit();
    tradEdit->setPlaceholderText("Saisir la traduction…");
    tradEdit->setText(block.translatedText);
    tradEdit->setMaximumHeight(65);

    // Sync en temps réel
    connect(origEdit, &QTextEdit::textChanged, this, [this, origEdit, bid]() {
        ImagePage* p = currentPage();
        if (!p) return;
        for (auto& b : p->blocks)
            if (b.id == bid) { b.originalText = origEdit->toPlainText(); break; }
    });
    connect(tradEdit, &QTextEdit::textChanged, this, [this, tradEdit, bid]() {
        ImagePage* p = currentPage();
        if (!p) return;
        for (auto& b : p->blocks)
            if (b.id == bid) { b.translatedText = tradEdit->toPlainText(); break; }
    });

    vl->addLayout(hl);
    vl->addWidget(lblOrig);
    vl->addWidget(origEdit);
    vl->addWidget(lblTrad);
    vl->addWidget(tradEdit);

    QVBoxLayout* ocrLayout = ui->ocrLayout;
    int pos = ocrLayout->count() - 1;
    ocrLayout->insertWidget(pos, frame);
    m_blockWidgets[bid] = frame;
}

void OCRwindow::clearBlockWidgets()
{
    for (auto* w : m_blockWidgets)
        w->deleteLater();
    m_blockWidgets.clear();
}

void OCRwindow::removeBlockFromUI(int id)
{
    if (m_blockWidgets.contains(id)) {
        QWidget* w = m_blockWidgets.take(id);
        ui->ocrLayout->removeWidget(w);
        w->deleteLater();
    }
}

void OCRwindow::onBlockDeleted(int id)
{
    ImagePage* page = currentPage();
    if (!page) return;

    page->blocks.erase(
        std::remove_if(page->blocks.begin(), page->blocks.end(),
                       [id](const TextBlock& b){ return b.id == id; }),
        page->blocks.end()
    );

    removeBlockFromUI(id);

    auto* canvas = qobject_cast<ImageCanvas*>(ui->imageCanvas);
    if (canvas) canvas->removeBlock(id);

    ui->lblStatus->setText(
        QString("Zone #%1 supprimée. %2 bloc(s) restant(s).")
            .arg(id).arg(page->blocks.size()));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Sauvegarde & Export
// ─────────────────────────────────────────────────────────────────────────────

void OCRwindow::on_btnSave_clicked()
{
    if (!m_project) return;
    m_project->save();
    ui->lblStatus->setText("Sauvegardé → " + m_project->rootPath + "/project.json");
}

void OCRwindow::on_btnExportJSON_clicked()
{
    ImagePage* page = currentPage();
    if (!page || page->blocks.empty()) {
        QMessageBox::information(this, "Export", "Aucun bloc à exporter."); return;
    }
    QString path = QFileDialog::getSaveFileName(this, "Exporter JSON", "", "JSON (*.json)");
    if (path.isEmpty()) return;

    Exporter exp;
    QFileInfo fi(currentImagePath());
    if (exp.exportJSON(page->blocks, fi.fileName(), path))
        ui->lblStatus->setText("JSON exporté → " + path);
    else
        QMessageBox::warning(this, "Erreur", "Échec de l'export JSON.");
}

void OCRwindow::on_btnExportPNG_clicked()
{
    ImagePage* page = currentPage();
    if (!page || page->blocks.empty()) {
        QMessageBox::information(this, "Export", "Aucun bloc à exporter."); return;
    }
    QString path = QFileDialog::getSaveFileName(this, "Exporter image rendue", "", "PNG (*.png)");
    if (path.isEmpty()) return;

    QImage base(currentImagePath());
    Exporter exp;
    QImage rendered = exp.renderImage(base, page->blocks);
    if (rendered.save(path))
        ui->lblStatus->setText("PNG exporté → " + path);
    else
        QMessageBox::warning(this, "Erreur", "Échec de l'export PNG.");
}

void OCRwindow::on_btnExportTXT_clicked()
{
    ImagePage* page = currentPage();
    if (!page || page->blocks.empty()) {
        QMessageBox::information(this, "Export", "Aucun bloc à exporter."); return;
    }
    QString path = QFileDialog::getSaveFileName(this, "Exporter TXT", "", "Texte (*.txt)");
    if (path.isEmpty()) return;

    QFileInfo fi(currentImagePath());
    Exporter exp;
    if (exp.exportTXT(page->blocks, fi.fileName(), path))
        ui->lblStatus->setText("TXT exporté → " + path);
    else
        QMessageBox::warning(this, "Erreur", "Échec de l'export TXT.");
}