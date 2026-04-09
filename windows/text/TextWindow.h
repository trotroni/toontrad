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
    void blocksReordered(const QList<int>& newIdOrder);

private slots:
    void onRowsMoved(const QModelIndex& parent,
                     int start, int end,
                     const QModelIndex& destination,
                     int row);

private:
    Ui::TextWindow* ui;
    QListWidget*    m_list   = nullptr;

    QMap<int, TextBlock> m_blockData;
    QList<int>           m_order;

    bool   m_dragInProgress = false;
    QPoint m_dragStartPos;
    int    m_dragSourceRow  = -1;

    void clearPanels();
    void buildWidget(int bid);
    void rebuildAllWidgets();

    bool eventFilter(QObject* obj, QEvent* event) override;
};

#endif // TEXTWINDOW_H
