#include "TextWindow.h"
#include "ui_TextWindow.h"
#include "../../core/models.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QTextEdit>
#include <QComboBox>
#include <QScrollArea>
#include <QScrollBar>
#include <QDebug>

TextWindow::TextWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::TextWindow)
{
    ui->setupUi(this);
    m_scrollWidget = ui->scrollContent;
    m_scrollLayout = qobject_cast<QVBoxLayout*>(m_scrollWidget->layout());
}

TextWindow::~TextWindow() { delete ui; }

// ─────────────────────────────────────────────────────────────────────────────

void TextWindow::clearPanels()
{
    for (auto* w : m_panels)
        w->deleteLater();
    m_panels.clear();

    // Supprime tous les items sauf le spacer final
    while (m_scrollLayout->count() > 1) {
        QLayoutItem* item = m_scrollLayout->takeAt(0);
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
}

void TextWindow::setBlocks(const std::vector<TextBlock>& blocks)
{
    clearPanels();
    for (const auto& b : blocks)
        addBubblePanel(b);

    ui->lblPage->setText(
        QString("%1 bulle(s)").arg(blocks.size()));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Un panneau par bulle : RAW + TRAD + STATUT + NOTES
// ─────────────────────────────────────────────────────────────────────────────

void TextWindow::addBubblePanel(const TextBlock& block)
{
    int bid = block.id;

    QFrame* frame = new QFrame(m_scrollWidget);
    frame->setFrameShape(QFrame::Box);
    frame->setObjectName(QString("panel_%1").arg(bid));

    QVBoxLayout* vl = new QVBoxLayout(frame);
    vl->setContentsMargins(6, 6, 6, 6);
    vl->setSpacing(4);

    // ── En-tête ───────────────────────────────────────────────────────────────
    QHBoxLayout* hl = new QHBoxLayout();

    QLabel* idLbl = new QLabel(
        QString("<b>Bulle #%1</b> &nbsp; <span style='color:#888; font-size:10px;'>"
                "conf: %2% &nbsp;|&nbsp; %3,%4 %5×%6</span>")
            .arg(bid)
            .arg(static_cast<int>(block.confidence * 100))
            .arg(block.boundingBox.x()).arg(block.boundingBox.y())
            .arg(block.boundingBox.width()).arg(block.boundingBox.height()));
    idLbl->setTextFormat(Qt::RichText);

    hl->addWidget(idLbl);
    hl->addStretch();
    vl->addLayout(hl);

    // ── RAW ───────────────────────────────────────────────────────────────────
    QLabel* rawLbl = new QLabel("RAW :");
    rawLbl->setStyleSheet("font-size: 11px; color: #888;");
    QTextEdit* rawEdit = new QTextEdit();
    rawEdit->setPlainText(block.originalText);
    rawEdit->setFixedHeight(60);
    rawEdit->setObjectName(QString("raw_%1").arg(bid));
    vl->addWidget(rawLbl);
    vl->addWidget(rawEdit);

    // ── TRAD ──────────────────────────────────────────────────────────────────
    QLabel* tradLbl = new QLabel("TRAD FR :");
    tradLbl->setStyleSheet("font-size: 11px; color: #888;");
    QTextEdit* tradEdit = new QTextEdit();
    tradEdit->setPlainText(block.translatedText);
    tradEdit->setFixedHeight(60);
    tradEdit->setPlaceholderText("Saisir la traduction…");
    tradEdit->setObjectName(QString("trad_%1").arg(bid));
    vl->addWidget(tradLbl);
    vl->addWidget(tradEdit);

    // ── STATUT ────────────────────────────────────────────────────────────────
    QHBoxLayout* statusRow = new QHBoxLayout();
    QLabel* statusLbl = new QLabel("Statut :");
    statusLbl->setStyleSheet("font-size: 11px; color: #888;");
    QComboBox* statusBox = new QComboBox();
    statusBox->addItems(ToonTrad::statusList());
    int si = ToonTrad::statusList().indexOf(block.status);
    if (si >= 0) statusBox->setCurrentIndex(si);
    statusRow->addWidget(statusLbl);
    statusRow->addWidget(statusBox);
    statusRow->addStretch();
    vl->addLayout(statusRow);

    // ── NOTES ─────────────────────────────────────────────────────────────────
    QLabel* notesLbl = new QLabel("Notes :");
    notesLbl->setStyleSheet("font-size: 11px; color: #888;");
    QTextEdit* notesEdit = new QTextEdit();
    notesEdit->setPlainText(block.notes);
    notesEdit->setFixedHeight(45);
    notesEdit->setPlaceholderText("Notes, remarques…");
    vl->addWidget(notesLbl);
    vl->addWidget(notesEdit);

    // ── Connexions live ───────────────────────────────────────────────────────
    auto emitUpdate = [this, bid, tradEdit, statusBox, notesEdit]() {
        emit blockUpdated(bid,
                          tradEdit->toPlainText(),
                          statusBox->currentText(),
                          notesEdit->toPlainText());
    };

    connect(tradEdit,  &QTextEdit::textChanged,   this, emitUpdate);
    connect(notesEdit, &QTextEdit::textChanged,   this, emitUpdate);
    connect(statusBox, &QComboBox::currentTextChanged,
            this, [emitUpdate](const QString&) { emitUpdate(); });

    // Clic sur l'en-tête → sélection sync vers ImageWindow
    connect(idLbl, &QLabel::linkActivated, this, [this, bid](const QString&) {
        emit blockSelected(bid);
    });
    // Clic sur le frame → sélection
    frame->installEventFilter(this);
    frame->setProperty("bubbleId", bid);

    // Insère avant le spacer
    m_scrollLayout->insertWidget(m_scrollLayout->count() - 1, frame);
    m_panels[bid] = frame;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Scroll + surlignage
// ─────────────────────────────────────────────────────────────────────────────

void TextWindow::scrollToBlock(int id)
{
    if (!m_panels.contains(id)) return;

    QWidget* panel = m_panels[id];

    // Remet tous à normal
    for (auto* w : m_panels)
        w->setStyleSheet("");

    // Surligne le panneau sélectionné
    panel->setStyleSheet("QFrame { border: 2px solid #FFD700; }");

    // Scroll vers ce panneau
    ui->scrollArea->ensureWidgetVisible(panel);
}

bool TextWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget* w = qobject_cast<QWidget*>(obj);
        if (w && w->property("bubbleId").isValid())
            emit blockSelected(w->property("bubbleId").toInt());
    }
    return QMainWindow::eventFilter(obj, event);
}
