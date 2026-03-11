#ifndef OCRWINDOW_H
#define OCRWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QMap>
#include <vector>
#include "../../core/OCRConfig.h"
#include "../../core/ProjectManager.h"
#include "../../core/TextBlock.h"

QT_BEGIN_NAMESPACE
namespace Ui { class OCRwindow; }
QT_END_NAMESPACE

class OCRwindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit OCRwindow(Project* project, const OCRConfig& config,
                       QWidget* parent = nullptr);
    ~OCRwindow();

private slots:
    void on_btnPrev_clicked();
    void on_btnNext_clicked();
    void on_btnRunOCR_clicked();
    void on_btnExportJSON_clicked();
    void on_btnExportPNG_clicked();
    void on_btnExportTXT_clicked();
    void on_btnSave_clicked();

    void onBlockDeleted(int id);

private:
    Ui::OCRwindow* ui;
    Project*       m_project;
    OCRConfig      m_config;
    int            m_currentPageIndex = 0;

    QMap<int, QWidget*> m_blockWidgets;

    void loadPage(int index);
    void displayImage();
    void displayBlocks();
    void updateNavButtons();

    void addBlockWidget(const TextBlock& block);
    void clearBlockWidgets();
    void removeBlockFromUI(int id);

    ImagePage* currentPage();
    QString    currentImagePath();
};

#endif // OCRWINDOW_H
