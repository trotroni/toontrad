#include "ImageWindow.h"
#include "ui_ImageWindow.h"
#include "../../core/OCRManager.h"
#include "../../core/Exporter.h"
#include "../../gui/canvas/ImageCanvas.h"
#include "../../config.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QApplication>
#include <QDebug>

// ─────────────────────────────────────────────────────────────────────────────
//  Constructeur
// ─────────────────────────────────────────────────────────────────────────────

ImageWindow::ImageWindow(Project* project, const OCRConfig& config, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::ImageWindow)
    , m_project(project)
    , m_config(config)
{
    ui->setupUi(this);
    setWindowTitle("ToonTrad — " + project->name + " [Image]");

    // Remplace le QGraphicsView du .ui par notre ImageCanvas custom
    auto* oldCanvas = ui->imageCanvas;
    auto* canvas    = new ImageCanvas(ui->centralwidget);
    oldCanvas->parentWidget()->layout()->replaceWidget(oldCanvas, canvas);
    oldCanvas->deleteLater();
    ui->imageCanvas = canvas;

    // Connexions canvas
    connect(canvas, &ImageCanvas::blockSelected,
            this,   &ImageWindow::onBlockSelectedOnCanvas);
    connect(canvas, &ImageCanvas::blockDeleteRequested,
            this,   &ImageWindow::onBlockDeleteRequested);
    connect(canvas, &ImageCanvas::addBubbleRequested,
            this,   &ImageWindow::onAddBubbleRequested);
    connect(canvas, &ImageCanvas::dragBubbleRequested,
            this,   &ImageWindow::onDragBubbleRequested);

    if (!project->pages.isEmpty())
        loadPage(0);
    else
        ui->lblStatus->setText("Aucune image dans ce projet.");
}

ImageWindow::~ImageWindow() { delete ui; }

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

ImagePage* ImageWindow::currentPage()
{
    if (!m_project || m_project->pages.isEmpty()) return nullptr;
    if (m_currentPageIndex < 0 ||
        m_currentPageIndex >= m_project->pages.size()) return nullptr;
    return &m_project->pages[m_currentPageIndex];
}

QString ImageWindow::currentImagePath()
{
    ImagePage* p = currentPage();
    return p ? m_project->absolutePath(*p) : QString();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Navigation
// ─────────────────────────────────────────────────────────────────────────────

void ImageWindow::loadPage(int index)
{
    if (currentPage() && currentPage()->ocrDone)
        m_project->save();

    m_currentPageIndex = index;
    displayImage();
    displayBlocks();
    updateNavButtons();

    emit pageChanged(index);
}

void ImageWindow::displayImage()
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

void ImageWindow::displayBlocks()
{
    auto* canvas   = qobject_cast<ImageCanvas*>(ui->imageCanvas);
    ImagePage* page = currentPage();
    if (!page || !canvas) return;

    canvas->setBlocks(page->blocks);
    emit blocksChanged(page->blocks);
}

void ImageWindow::updateNavButtons()
{
    int total = m_project->pages.size();
    ui->btnPrev->setEnabled(m_currentPageIndex > 0);
    ui->btnNext->setEnabled(m_currentPageIndex < total - 1);
    ui->lblPage->setText(
        QString("Page %1 / %2  —  %3")
            .arg(m_currentPageIndex + 1)
            .arg(total)
            .arg(QFileInfo(currentImagePath()).fileName()));
}

void ImageWindow::on_btnPrev_clicked()
{
    if (m_currentPageIndex > 0)
        loadPage(m_currentPageIndex - 1);
}

void ImageWindow::on_btnNext_clicked()
{
    if (m_currentPageIndex < m_project->pages.size() - 1)
        loadPage(m_currentPageIndex + 1);
}

// ─────────────────────────────────────────────────────────────────────────────
//  OCR
// ─────────────────────────────────────────────────────────────────────────────

void ImageWindow::runOCR()
{
    ImagePage* page = currentPage();
    if (!page) return;

    QString imgPath = currentImagePath();
    if (!QFile::exists(imgPath)) {
        QMessageBox::warning(this, "Erreur", "Image introuvable : " + imgPath);
        return;
    }

    QString pyErr;
    if (!OCRManager::checkPythonAvailable(&pyErr)) {
        QMessageBox::critical(this, "Python introuvable",
            pyErr + "\n\nVérifiez les chemins dans Paramètres.");
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

    auto blocks   = ocr.runOCR(imgPath, m_config);
    page->blocks  = std::move(blocks);
    page->ocrDone = !page->blocks.empty();

    displayBlocks();
    ui->lblStatus->setText(
        QString("OCR terminé : %1 bloc(s)").arg(page->blocks.size()));
    ui->btnRunOCR->setEnabled(true);
}

void ImageWindow::on_btnRunOCR_clicked() { runOCR(); }

// ─────────────────────────────────────────────────────────────────────────────
//  Sync depuis TextWindow
// ─────────────────────────────────────────────────────────────────────────────

void ImageWindow::onBlockUpdated(int id, const QString& trad,
                                  const QString& status, const QString& notes)
{
    ImagePage* page = currentPage();
    if (!page) return;
    for (auto& b : page->blocks) {
        if (b.id == id) {
            b.translatedText = trad;
            b.status         = status;
            b.notes          = notes;
            break;
        }
    }
    // Refresh labels sur canvas
    auto* canvas = qobject_cast<ImageCanvas*>(ui->imageCanvas);
    if (canvas) canvas->setBlocks(page->blocks);
}

void ImageWindow::highlightBlock(int id)
{
    auto* canvas = qobject_cast<ImageCanvas*>(ui->imageCanvas);
    if (canvas) canvas->highlightBlock(id);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Interactions canvas
// ─────────────────────────────────────────────────────────────────────────────

void ImageWindow::onBlockSelectedOnCanvas(int id)
{
    emit blockSelected(id);
}

void ImageWindow::onBlockDeleteRequested(int id)
{
    ImagePage* page = currentPage();
    if (!page) return;

    page->blocks.erase(
        std::remove_if(page->blocks.begin(), page->blocks.end(),
                       [id](const TextBlock& b) { return b.id == id; }),
        page->blocks.end());

    displayBlocks();
    ui->lblStatus->setText(QString("Bulle #%1 supprimée.").arg(id));
}

void ImageWindow::onAddBubbleRequested(QPointF scenePos)
{
    ImagePage* page = currentPage();
    if (!page) return;

    int nextId = 1;
    for (const auto& b : page->blocks)
        nextId = qMax(nextId, b.id + 1);

    QRect rect(static_cast<int>(scenePos.x()) - 75,
               static_cast<int>(scenePos.y()) - 40, 150, 80);
    TextBlock b(nextId, rect, "");
    page->blocks.push_back(b);

    displayBlocks();
    ui->lblStatus->setText(QString("Bulle #%1 ajoutée.").arg(nextId));
}

void ImageWindow::onDragBubbleRequested(QRectF rect)
{
    ImagePage* page = currentPage();
    if (!page) return;

    int nextId = 1;
    for (const auto& b : page->blocks)
        nextId = qMax(nextId, b.id + 1);

    QRect r(static_cast<int>(rect.x()), static_cast<int>(rect.y()),
            static_cast<int>(rect.width()), static_cast<int>(rect.height()));
    TextBlock b(nextId, r, "");
    page->blocks.push_back(b);

    displayBlocks();
    ui->lblStatus->setText(QString("Bulle #%1 ajoutée par drag.").arg(nextId));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Export / Sauvegarde
// ─────────────────────────────────────────────────────────────────────────────

void ImageWindow::on_btnSave_clicked()
{
    if (m_project) {
        m_project->save();
        ui->lblStatus->setText("Sauvegardé → " + m_project->rootPath + "/project.json");
    }
}

void ImageWindow::on_btnExportTXT_clicked()
{
    ImagePage* page = currentPage();
    if (!page || page->blocks.empty()) {
        QMessageBox::information(this, "Export", "Aucun bloc."); return;
    }
    QString folder = QFileDialog::getExistingDirectory(this, "Dossier de sortie");
    if (folder.isEmpty()) return;
    Exporter exp;
    QString imgName = QFileInfo(currentImagePath()).fileName();
    exp.exportTXT(page->blocks, imgName, folder);
    ui->lblStatus->setText("TXT exporté → " + folder);
}

void ImageWindow::on_btnExportJSON_clicked()
{
    ImagePage* page = currentPage();
    if (!page || page->blocks.empty()) {
        QMessageBox::information(this, "Export", "Aucun bloc."); return;
    }
    QString path = QFileDialog::getSaveFileName(this, "Exporter JSON", "", "JSON (*.json)");
    if (path.isEmpty()) return;
    Exporter exp;
    QString imgName = QFileInfo(currentImagePath()).fileName();
    exp.exportJSON(page->blocks, imgName, path);
    ui->lblStatus->setText("JSON exporté → " + path);
}

void ImageWindow::on_btnExportPNG_clicked()
{
    ImagePage* page = currentPage();
    if (!page || page->blocks.empty()) {
        QMessageBox::information(this, "Export", "Aucun bloc."); return;
    }
    QString path = QFileDialog::getSaveFileName(this, "Exporter PNG", "", "PNG (*.png)");
    if (path.isEmpty()) return;
    QImage base(currentImagePath());
    Exporter exp;
    exp.exportPNG(base, page->blocks, path);
    ui->lblStatus->setText("PNG exporté → " + path);
}

void ImageWindow::on_btnExportPS_clicked()
{
    ImagePage* page = currentPage();
    if (!page || page->blocks.empty()) {
        QMessageBox::information(this, "Export", "Aucun bloc."); return;
    }
    QString path = QFileDialog::getSaveFileName(
        this, "Exporter JSON Photoshop", "", "JSON (*.json)");
    if (path.isEmpty()) return;
    Exporter exp;
    QString imgName = QFileInfo(currentImagePath()).fileName();
    exp.exportPhotoshopJSON(page->blocks, imgName, path);
    ui->lblStatus->setText("JSON Photoshop exporté → " + path);
}

void ImageWindow::on_btnExportAll_clicked()
{
    QString folder = QFileDialog::getExistingDirectory(
        this, "Dossier de sortie pour l'export complet");
    if (folder.isEmpty()) return;

    Exporter exp;
    for (const auto& page : m_project->pages) {
        if (page.blocks.empty()) continue;
        QString imgName = QFileInfo(m_project->absolutePath(page)).fileName();
        QString imgPath = m_project->absolutePath(page);

        exp.exportTXT(page.blocks, imgName, folder);
        exp.exportJSON(page.blocks, imgName,
                       folder + "/" + QFileInfo(imgName).completeBaseName() + ".json");
        exp.exportPhotoshopJSON(page.blocks, imgName,
                       folder + "/" + QFileInfo(imgName).completeBaseName() + "_ps.json");
        QImage base(imgPath);
        if (!base.isNull())
            exp.exportPNG(base, page.blocks,
                          folder + "/" + QFileInfo(imgName).completeBaseName() + "_trad.png");
    }

    // Fichier consolidé
    QString consolidated = folder + "/translations_final.txt";
    Exporter::exportConsolidated(folder, consolidated);

    ui->lblStatus->setText("Export complet → " + folder);
    QMessageBox::information(this, "Export terminé",
        "Tous les fichiers ont été exportés dans :\n" + folder);
}
