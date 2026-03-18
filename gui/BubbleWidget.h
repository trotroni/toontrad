#ifndef BUBBLEWIDGET_H
#define BUBBLEWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include "../core/Bubble.h"

class BubbleWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BubbleWidget(const Bubble& bubble, QWidget* parent = nullptr);

    // Lit les valeurs actuelles des champs et met à jour la bulle
    void syncToBubble(Bubble& bubble) const;

    int bubbleId() const { return m_id; }

    signals:
        // Bouton ✕ cliqué
        void deleteRequested(int id);

    // raw ou trad modifiés
    void textChanged();

private slots:
    void onTextChanged();

private:
    int          m_id;
    QTextEdit*   m_rawEdit;
    QTextEdit*   m_tradEdit;
    QComboBox*   m_statusBox;
};

#endif // BUBBLEWIDGET_H