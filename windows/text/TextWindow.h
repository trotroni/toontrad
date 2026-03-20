#ifndef TEXTWINDOW_H
#define TEXTWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QVBoxLayout>
#include <QListWidget>
#include <vector>
#include "../../core/TextBlock.h"

QT_BEGIN_NAMESPACE
namespace Ui { class TextWindow; }
QT_END_NAMESPACE

class TextWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TextWindow(QWidget* parent = nullptr);
    ~TextWindow();

    void setBlocks(const std::vector<TextBlock>& blocks);
    void scrollToBlock(int id);

signals:
    void blockUpdated(int id, const QString& trad,
                      const QString& status, const QString& notes);
    void blockSelected(int id);

    // Émis quand l'ordre des blocs change par drag
    void blocksReordered(const QList<int>& newIdOrder);

private slots:
    void onRowsMoved();

private:
    Ui::TextWindow*     ui;
    QListWidget*        m_list = nullptr;
    QMap<int, QWidget*> m_panels;

    void clearPanels();
    bool eventFilter(QObject* obj, QEvent* event) override;
    void addBubbleItem(const TextBlock& block);
};

#endif // TEXTWINDOW_H
