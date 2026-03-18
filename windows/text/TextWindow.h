#ifndef TEXTWINDOW_H
#define TEXTWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QVBoxLayout>
#include <vector>
#include "../../core/TextBlock.h"

QT_BEGIN_NAMESPACE
namespace Ui { class TextWindow; }
QT_END_NAMESPACE

class BubblePanel;

class TextWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit TextWindow(QWidget* parent = nullptr);
    ~TextWindow();

    // Reçoit la liste des blocs depuis ImageWindow
    void setBlocks(const std::vector<TextBlock>& blocks);

    // Scroll vers la bulle id et la surligne
    void scrollToBlock(int id);

signals:
    // Émis quand l'utilisateur modifie un champ
    void blockUpdated(int id, const QString& trad,
                      const QString& status, const QString& notes);

    // Émis quand l'utilisateur clique sur un bloc dans le panneau
    void blockSelected(int id);

private:
    Ui::TextWindow*       ui;
    QWidget*              m_scrollWidget  = nullptr;
    QVBoxLayout*          m_scrollLayout  = nullptr;
    QMap<int, QWidget*>   m_panels;

    void clearPanels();
    void addBubblePanel(const TextBlock& block);

    bool eventFilter(QObject* obj, QEvent* event) override;
};

#endif // TEXTWINDOW_H
