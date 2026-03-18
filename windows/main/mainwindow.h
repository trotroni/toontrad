#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QStringList>
#include <vector>
#include "../../core/Bubble.h"
#include "../../core/Exporter.h"
#include "../../gui/canvas/ImageCanvas.h"
#include "../../gui/widgets/BubbleWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onBtnPrev();
    void onBtnNext();
    void onBtnSave();
    void onBtnAdd();
    void onBtnOpenFolder();
    void onBtnReloadConfig();

    void onBubbleDeleteRequested(int id);
    void onBubbleAddRequested(QRect rect);
    void onTextChanged();

private:
    Ui::MainWindow* ui;

    // État
    QStringList          m_imageList;
    int                  m_currentIndex = 0;
    std::vector<Bubble>  m_bubbles;

    // Widgets dynamiques panneau droit
    QWidget*             m_scrollWidget  = nullptr;
    QVBoxLayout*         m_scrollLayout  = nullptr;
    QList<BubbleWidget*> m_bubbleWidgets;

    // Helpers
    void loadImages(const QString& folderPath);
    void loadCurrentImage();
    void buildRightPanel();
    void updateNavButtons();
    void syncBubbles();           // lit tous les BubbleWidget → m_bubbles
    void recalculateIds();
    void log(const QString& msg);

    QString currentImagePath() const;
};

#endif // MAINWINDOW_H