#include "ImageWindow.h"
#include "ui_ImageWindow.h"
#include "../../core/OCRManager.h"
#include "../../core/Exporter.h"
#include "../../gui/canvas/ImageCanvas.h"
#include "../../config.h"

#include <QFileInfo>
#include <QMessageBox>
#include <QApplication>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QProcess>
#include <QDir>
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

    // Synchronise m_autoOCR avec la checkbox
    m_autoOCR = ui->chkAutoOCR->isChecked();
    connect(ui->chkAutoOCR, &QCheckBox::toggled,
            this, [this](bool checked) {
        m_autoOCR = checked;
        ui->lblStatus->setText(
            checked ? "OCR automatique activé"
                    : "OCR manuel — dessinez une zone puis remplissez le RAW");
    });

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

int ImageWindow::nextBlockId()
{
    ImagePage* page = currentPage();
    if (!page || page->blocks.empty()) return 1;
    int maxId = 0;
    for (const auto& b : page->blocks)
        maxId = qMax(maxId, b.id);
    return maxId + 1;
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
    auto* canvas    = qobject_cast<ImageCanvas*>(ui->imageCanvas);
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
//  OCR — Relance sur les zones existantes
// ─────────────────────────────────────────────────────────────────────────────

void ImageWindow::on_btnRunOCR_clicked()
{
    ImagePage* page = currentPage();
    if (!page) return;

    if (page->blocks.empty()) {
        ui->lblStatus->setText(
            "Aucune zone définie. Dessinez des zones avec la souris.");
        return;
    }

    QString imgPath = currentImagePath();
    QString pyErr;
    if (!OCRManager::checkPythonAvailable(&pyErr)) {
        QMessageBox::critical(this, "Python introuvable", pyErr);
        return;
    }

    ui->btnRunOCR->setEnabled(false);
    ui->lblStatus->setText("Re-OCR en cours sur les zones existantes…");
    qApp->processEvents();

    QJsonArray rectsArr;
    for (const auto& b : page->blocks) {
        QJsonObject entry;
        entry["id"] = b.id;
        QJsonArray r;
        r << b.boundingBox.x() << b.boundingBox.y()
          << b.boundingBox.width() << b.boundingBox.height();
        entry["rect"] = r;
        rectsArr.append(entry);
    }

    QJsonObject args = m_config.toJson();
    args["mode"]          = "reocr";
    args["image_path"]    = imgPath;
    args["inner_ratio"]   = Config::innerRectRatio;
    args["tessdata_path"] = Config::tessdataPath;
    args["rects"]         = rectsArr;

    QString argsJson = QJsonDocument(args).toJson(QJsonDocument::Compact);

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(Config::pythonBin, {Config::detectScript, argsJson});

    if (!process.waitForStarted(10000) || !process.waitForFinished(120000)) {
        process.kill();
        QMessageBox::critical(this, "Erreur", "Timeout ou échec Python");
        ui->btnRunOCR->setEnabled(true);
        return;
    }

    QByteArray stderrData = process.readAllStandardError();
    if (!stderrData.isEmpty())
        qDebug() << "detect.py stderr:" << stderrData;

    if (process.exitCode() != 0) {
        QMessageBox::critical(this, "Erreur OCR", QString::fromUtf8(stderrData));
        ui->btnRunOCR->setEnabled(true);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(process.readAllStandardOutput());
    if (doc.isArray()) {
        for (const QJsonValue& v : doc.array()) {
            QJsonObject obj = v.toObject();
            int bid  = obj["id"].toInt();
            QString raw = obj["raw"].toString();
            for (auto& b : page->blocks) {
                if (b.id == bid) { b.originalText = raw; break; }
            }
        }
        page->ocrDone = true;
    }

    displayBlocks();
    ui->lblStatus->setText(
        QString("Re-OCR terminé : %1 zone(s) traitée(s)").arg(page->blocks.size()));
    ui->btnRunOCR->setEnabled(true);
}

// ─────────────────────────────────────────────────────────────────────────────
//  OCR sur un crop
// ─────────────────────────────────────────────────────────────────────────────

QString ImageWindow::runOCROnCrop(const QString& cropPath)
{
    QJsonObject args = m_config.toJson();
    args["mode"]          = "crop";
    args["image_path"]    = cropPath;
    args["inner_ratio"]   = Config::innerRectRatio;
    args["tessdata_path"] = Config::tessdataPath;

    QString argsJson = QJsonDocument(args).toJson(QJsonDocument::Compact);

    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(Config::pythonBin, {Config::detectScript, argsJson});

    if (!process.waitForStarted(10000) || !process.waitForFinished(30000)) {
        process.kill();
        return {};
    }

    if (process.exitCode() != 0) return {};

    QJsonDocument doc = QJsonDocument::fromJson(process.readAllStandardOutput());
    if (doc.isArray() && !doc.array().isEmpty())
        return doc.array().first().toObject()["raw"].toString();
    return {};
}

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
    auto* canvas = qobject_cast<ImageCanvas*>(ui->imageCanvas);
    if (canvas) canvas->setBlocks(page->blocks);
}

void ImageWindow::highlightBlock(int id)
{
    auto* canvas = qobject_cast<ImageCanvas*>(ui->imageCanvas);
    if (canvas) canvas->highlightBlock(id);
}

void ImageWindow::reorderBlocks(const QList<int>& newIdOrder)
{
    ImagePage* page = currentPage();
    if (!page) return;

    std::vector<TextBlock> reordered;
    reordered.reserve(page->blocks.size());
    for (int id : newIdOrder)
        for (const auto& b : page->blocks)
            if (b.id == id) { reordered.push_back(b); break; }

    for (int i = 0; i < (int)reordered.size(); ++i)
        reordered[i].id = i + 1;

    page->blocks = reordered;

    auto* canvas = qobject_cast<ImageCanvas*>(ui->imageCanvas);
    if (canvas) canvas->setBlocks(page->blocks);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Interactions canvas
// ─────────────────────────────────────────────────────────────────────────────

void ImageWindow::onBlockSelectedOnCanvas(int id) { emit blockSelected(id); }

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
    QRect rect(static_cast<int>(scenePos.x()) - 75,
               static_cast<int>(scenePos.y()) - 40, 150, 80);
    TextBlock b(nextBlockId(), rect, "");
    page->blocks.push_back(b);
    displayBlocks();
    ui->lblStatus->setText(
        QString("Bulle #%1 ajoutée (vide).").arg(b.id));
}

void ImageWindow::onDragBubbleRequested(QRectF rect)
{
    ImagePage* page = currentPage();
    if (!page) return;

    QString imgPath = currentImagePath();
    if (imgPath.isEmpty()) return;

    int bid = nextBlockId();
    QRect r(static_cast<int>(rect.x()), static_cast<int>(rect.y()),
            static_cast<int>(rect.width()), static_cast<int>(rect.height()));

    // ── Mode manuel : bulle vide, pas d'OCR ──────────────────────────────
    if (!m_autoOCR) {
        TextBlock b(bid, r, "");
        page->blocks.push_back(b);
        displayBlocks();
        ui->lblStatus->setText(
            QString("Bulle #%1 créée — remplissez le RAW manuellement.").arg(bid));
        return;
    }

    // ── Mode auto : OCR immédiat ──────────────────────────────────────────
    ui->lblStatus->setText(QString("OCR en cours sur la zone #%1…").arg(bid));
    qApp->processEvents();

    QString pyErr;
    if (!OCRManager::checkPythonAvailable(&pyErr)) {
        TextBlock b(bid, r, "");
        page->blocks.push_back(b);
        displayBlocks();
        ui->lblStatus->setText("Python non dispo — bulle ajoutée sans texte.");
        return;
    }

    QImage fullImg(imgPath);
    QRect clampedRect = r.intersected(fullImg.rect());
    if (clampedRect.isEmpty()) return;

    QImage crop = fullImg.copy(clampedRect);
    QString tmpPath = QDir::tempPath() +
                      QString("/toontrad_crop_%1.png").arg(bid);
    crop.save(tmpPath);

    QString rawText = runOCROnCrop(tmpPath);
    QFile::remove(tmpPath);

    TextBlock b(bid, clampedRect, rawText);
    page->blocks.push_back(b);
    page->ocrDone = true;

    displayBlocks();

    if (rawText.isEmpty())
        ui->lblStatus->setText(
            QString("Bulle #%1 ajoutée — aucun texte détecté.").arg(bid));
    else
        ui->lblStatus->setText(
            QString("Bulle #%1 : \"%2\"").arg(bid).arg(rawText.left(40)));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Sauvegarde & Export
// ─────────────────────────────────────────────────────────────────────────────

void ImageWindow::on_btnSave_clicked()
{
    if (m_project) {
        m_project->save();
        ui->lblStatus->setText("Sauvegardé → " + m_project->rootPath);
    }
}

void ImageWindow::on_btnExportTXT_clicked()
{
    ImagePage* page = currentPage();
    if (!page || page->blocks.empty()) {
        QMessageBox::information(this, "Export", "Aucun bloc."); return;
    }
    QString folder = m_project->rootPath + "/output";
    QDir().mkpath(folder);
    Exporter exp;
    exp.exportTXT(page->blocks, QFileInfo(currentImagePath()).fileName(), folder);
    ui->lblStatus->setText("TXT → " + folder);
}

void ImageWindow::on_btnExportJSON_clicked()
{
    ImagePage* page = currentPage();
    if (!page || page->blocks.empty()) {
        QMessageBox::information(this, "Export", "Aucun bloc."); return;
    }
    QString folder = m_project->rootPath + "/output";
    QDir().mkpath(folder);
    QString path = folder + "/" + QFileInfo(currentImagePath()).completeBaseName() + ".json";
    Exporter exp;
    exp.exportJSON(page->blocks, QFileInfo(currentImagePath()).fileName(), path);
    ui->lblStatus->setText("JSON → " + path);
}

void ImageWindow::on_btnExportPNG_clicked()
{
    ImagePage* page = currentPage();
    if (!page || page->blocks.empty()) {
        QMessageBox::information(this, "Export", "Aucun bloc."); return;
    }
    QString folder = m_project->rootPath + "/renders";
    QDir().mkpath(folder);
    QString path = folder + "/" + QFileInfo(currentImagePath()).completeBaseName() + "_trad.png";
    QImage img(currentImagePath());
    Exporter exp;
    exp.exportPNG(img, page->blocks, path);
    ui->lblStatus->setText("PNG → " + path);
}

void ImageWindow::on_btnExportPS_clicked()
{
    ImagePage* page = currentPage();
    if (!page || page->blocks.empty()) {
        QMessageBox::information(this, "Export", "Aucun bloc."); return;
    }
    QString folder = m_project->rootPath + "/photoshop";
    QDir().mkpath(folder);
    QString path = folder + "/" + QFileInfo(currentImagePath()).completeBaseName() + "_ps.json";
    Exporter exp;
    exp.exportPhotoshopJSON(page->blocks,
                             QFileInfo(currentImagePath()).fileName(), path);
    ui->lblStatus->setText("PS JSON → " + path);
}

void ImageWindow::on_btnExportAll_clicked()
{
    QString outFolder = m_project->rootPath + "/output";
    QString renFolder = m_project->rootPath + "/renders";
    QString psFolder  = m_project->rootPath + "/photoshop";
    QDir().mkpath(outFolder);
    QDir().mkpath(renFolder);
    QDir().mkpath(psFolder);

    Exporter exp;
    int count = 0;
    for (const auto& page : m_project->pages) {
        if (page.blocks.empty()) continue;
        QString imgPath = m_project->absolutePath(page);
        QString imgName = QFileInfo(imgPath).fileName();
        QString base    = QFileInfo(imgPath).completeBaseName();

        exp.exportTXT(page.blocks, imgName, outFolder);
        exp.exportJSON(page.blocks, imgName, outFolder + "/" + base + ".json");
        exp.exportPhotoshopJSON(page.blocks, imgName, psFolder + "/" + base + "_ps.json");
        QImage img(imgPath);
        if (!img.isNull())
            exp.exportPNG(img, page.blocks, renFolder + "/" + base + "_trad.png");
        count++;
    }

    Exporter::exportConsolidated(outFolder, outFolder + "/translations_final.txt");

    ui->lblStatus->setText(
        QString("Export complet : %1 page(s) → %2").arg(count).arg(m_project->rootPath));
    QMessageBox::information(this, "Export terminé",
        QString("%1 page(s) exportée(s) dans :\n"
                "• output/  (TXT, JSON, consolidé)\n"
                "• renders/ (PNG)\n"
                "• photoshop/ (JSON PS)").arg(count));
}