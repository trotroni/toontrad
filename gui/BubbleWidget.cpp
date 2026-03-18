#include "BubbleWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>

BubbleWidget::BubbleWidget(const Bubble& bubble, QWidget* parent)
    : QWidget(parent)
    , m_id(bubble.id)
{
    // ── Conteneur avec bordure (comme QFrame box Python) ─────────────────────
    QFrame* frame = new QFrame(this);
    frame->setFrameShape(QFrame::Box);
    frame->setFrameShadow(QFrame::Plain);

    QVBoxLayout* frameLayout = new QVBoxLayout(frame);
    frameLayout->setContentsMargins(6, 6, 6, 6);
    frameLayout->setSpacing(4);

    // ── En-tête : ID | outer rect | inner rect | bouton ✕ ───────────────────
    QHBoxLayout* headerLayout = new QHBoxLayout();

    QLabel* idLabel = new QLabel(
        QString("ID %1 | Outer: %2,%3,%4,%5 | Inner: %6,%7,%8,%9")
            .arg(bubble.id)
            .arg(bubble.rect.x()).arg(bubble.rect.y())
            .arg(bubble.rect.width()).arg(bubble.rect.height())
            .arg(bubble.innerRect.x()).arg(bubble.innerRect.y())
            .arg(bubble.innerRect.width()).arg(bubble.innerRect.height())
    );
    idLabel->setStyleSheet("font-size: 10px; color: #888;");

    QPushButton* btnDelete = new QPushButton("✕");
    btnDelete->setFixedSize(22, 22);
    btnDelete->setToolTip("Supprimer cette bulle");

    connect(btnDelete, &QPushButton::clicked, this, [this]() {
        emit deleteRequested(m_id);
    });

    headerLayout->addWidget(idLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(btnDelete);

    // ── Champ RAW ────────────────────────────────────────────────────────────
    QLabel* rawLabel = new QLabel("RAW :");
    rawLabel->setStyleSheet("font-size: 11px;");

    m_rawEdit = new QTextEdit();
    m_rawEdit->setPlainText(bubble.raw);
    m_rawEdit->setFixedHeight(60);
    m_rawEdit->setPlaceholderText("Texte OCR brut…");

    // ── Champ TRAD ───────────────────────────────────────────────────────────
    QLabel* tradLabel = new QLabel("TRAD FR :");
    tradLabel->setStyleSheet("font-size: 11px;");

    m_tradEdit = new QTextEdit();
    m_tradEdit->setPlainText(bubble.trad);
    m_tradEdit->setFixedHeight(60);
    m_tradEdit->setPlaceholderText("Saisir la traduction…");

    // ── Statut ───────────────────────────────────────────────────────────────
    m_statusBox = new QComboBox();
    m_statusBox->addItems(STATUTS);
    int idx = STATUTS.indexOf(bubble.status);
    if (idx >= 0) m_statusBox->setCurrentIndex(idx);

    // ── Assemblage ───────────────────────────────────────────────────────────
    frameLayout->addLayout(headerLayout);
    frameLayout->addWidget(rawLabel);
    frameLayout->addWidget(m_rawEdit);
    frameLayout->addWidget(tradLabel);
    frameLayout->addWidget(m_tradEdit);
    frameLayout->addWidget(m_statusBox);

    // Layout principal du widget
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 4);
    mainLayout->addWidget(frame);

    // ── Signaux live (identique à textChanged.connect en Python) ─────────────
    connect(m_rawEdit,  &QTextEdit::textChanged, this, &BubbleWidget::onTextChanged);
    connect(m_tradEdit, &QTextEdit::textChanged, this, &BubbleWidget::onTextChanged);
}

void BubbleWidget::onTextChanged()
{
    emit textChanged();
}

void BubbleWidget::syncToBubble(Bubble& bubble) const
{
    bubble.raw    = m_rawEdit->toPlainText().simplified();
    bubble.trad   = m_tradEdit->toPlainText().simplified();
    bubble.status = m_statusBox->currentText();
}