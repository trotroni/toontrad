#ifndef IMAGEWINDOW_H
#define IMAGEWINDOW_H

#include <QMainWindow>
#include <vector>
#include "../../core/TextBlock.h"
#include "../../core/OCRConfig.h"
#include "../../core/ProjectManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class ImageWindow; }
QT_END_NAMESPACE

class ImageWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ImageWindow(Project* project, const OCRConfig& config,
                         QWidget* parent = nullptr);
    ~ImageWindow();

    // Appelé par TextWindow pour synchroniser
    void highlightBlock(int id);
    void reorderBlocks(const QList<int>& newIdOrder);

signals:
    // Émis vers TextWindow
    void blockSelected(int id);
    void blocksChanged(const std::vector<TextBlock>& blocks);
    void pageChanged(int pageIndex);

public slots:
    // Reçoit les mises à jour du TextWindow
    void onBlockUpdated(int id, const QString& trad,
                        const QString& status, const QString& notes);

private slots:
    void on_btnPrev_clicked();
    void on_btnNext_clicked();
    void on_btnRunOCR_clicked();
    void on_btnSave_clicked();
    void on_btnExportTXT_clicked();
    void on_btnExportJSON_clicked();
    void on_btnExportPNG_clicked();
    void on_btnExportPS_clicked();
    void on_btnExportAll_clicked();

    void onBlockDeleteRequested(int id);
    void onAddBubbleRequested(QPointF scenePos);
    void onDragBubbleRequested(QRectF rect);
    void onBlockSelectedOnCanvas(int id);

private:
    Ui::ImageWindow* ui;
    Project*   m_project;
    OCRConfig  m_config;
    int        m_currentPageIndex = 0;

    void loadPage(int index);
    void displayImage();
    void displayBlocks();
    void updateNavButtons();

    int     nextBlockId();
    QString runOCROnCrop(const QString& cropPath);  // OCR direct sur un crop
    void runOCR();

    ImagePage* currentPage();
    QString    currentImagePath();
};

#endif // IMAGEWINDOW_H
