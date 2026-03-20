#include "TextWindow.h"
#include "ui_TextWindow.h"
#include "../../core/models.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QTextEdit>
#include <QComboBox>
#include <QListWidgetItem>
#include <QDebug>

TextWindow::TextWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::TextWindow)
{
    ui->setupUi(this);

    // Remplace le QScrollArea du .ui par un QListWidget draggable
    m_list = new QListWidget(ui->centralwidget);
    m_list->setDragDropMode(QAbstractItemView::InternalMove);
    m_list->setDefaultDropAction(Qt::MoveAction);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setSpacing(2);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    // Remplace le scrollArea dans le layout
    auto* rootLayout = qobject_cast<QVBoxLayout*>(
        ui->centralwidget->layout());
    // Supprime l'ancien scrollArea (index 1 dans le layout)
    QLayoutItem* old = rootLayout->takeAt(1);
    if (old) { if (old->widget()) old->widget()->hide(); delete old; }
    rootLayout->insertWidget(1, m_list);

    // Signal quand l'ordre change par drag
    connect(m_list->model(), &QAbstractItemModel::rowsMoved,
            this, &TextWindow::onRowsMoved);
}

TextWindow::~TextWindow() { delete ui; }

// ─────────────────────────────────────────────────────────────────────────────

void TextWindow::clearPanels()
{
    m_list->clear();
    m_panels.clear();
}

void TextWindow::setBlocks(const std::vector<TextBlock>& blocks)
{
    clearPanels();
    for (const auto& b : blocks)
        addBubbleItem(b);
    ui->lblPage->setText(QString("%1 bulle(s)").arg(blocks.size()));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Un item par bulle — widget dans le QListWidget
// ─────────────────────────────────────────────────────────────────────────────

void TextWindow::addBubbleItem(const TextBlock& block)
{
    int bid = block.id;

    // ── Widget contenu dans l'item ─────────────────────────────────────────
    QFrame* frame = new QFrame();
    frame->setFrameShape(QFrame::Box);
    frame->setObjectName(QString("panel_%1").arg(bid));
    frame->setProperty("bubbleId", bid);

    QVBoxLayout* vl = new QVBoxLayout(frame);
    vl->setContentsMargins(6, 6, 6, 6);
    vl->setSpacing(4);

    // ── En-tête avec poignée de drag ──────────────────────────────────────
    QHBoxLayout* hl = new QHBoxLayout();

    QLabel* dragHandle = new QLabel("⠿");
    dragHandle->setStyleSheet("color: #555; font-size: 16px; padding: 0 4px;");
    dragHandle->setToolTip("Glisser pour réordonner");

    QLabel* idLbl = new QLabel(
        QString("<b>Bulle #%1</b> &nbsp;"
                "<span style='color:#888; font-size:10px;'>"
                "conf:%2% &nbsp;|&nbsp; %3,%4 %5×%6</span>")
            .arg(bid)
            .arg(static_cast<int>(block.confidence * 100))
            .arg(block.boundingBox.x()).arg(block.boundingBox.y())
            .arg(block.boundingBox.width()).arg(block.boundingBox.height()));
    idLbl->setTextFormat(Qt::RichText);

    hl->addWidget(dragHandle);
    hl->addWidget(idLbl);
    hl->addStretch();
    vl->addLayout(hl);

    // ── RAW ──────────────────────────────────────────────────────────────
    QLabel* rawLbl = new QLabel("RAW :");
    rawLbl->setStyleSheet("font-size:11px; color:#888;");
    QTextEdit* rawEdit = new QTextEdit();
    rawEdit->setPlainText(block.originalText);
    rawEdit->setFixedHeight(55);
    vl->addWidget(rawLbl);
    vl->addWidget(rawEdit);

    // ── TRAD ─────────────────────────────────────────────────────────────
    QLabel* tradLbl = new QLabel("TRAD FR :");
    tradLbl->setStyleSheet("font-size:11px; color:#888;");
    QTextEdit* tradEdit = new QTextEdit();
    tradEdit->setPlainText(block.translatedText);
    tradEdit->setFixedHeight(55);
    tradEdit->setPlaceholderText("Saisir la traduction…");
    vl->addWidget(tradLbl);
    vl->addWidget(tradEdit);

    // ── STATUT ───────────────────────────────────────────────────────────
    QHBoxLayout* statusRow = new QHBoxLayout();
    QLabel* statusLbl = new QLabel("Statut :");
    statusLbl->setStyleSheet("font-size:11px; color:#888;");
    QComboBox* statusBox = new QComboBox();
    statusBox->addItems(ToonTrad::statusList());
    int si = ToonTrad::statusList().indexOf(block.status);
    if (si >= 0) statusBox->setCurrentIndex(si);
    statusRow->addWidget(statusLbl);
    statusRow->addWidget(statusBox);
    statusRow->addStretch();
    vl->addLayout(statusRow);

    // ── NOTES ────────────────────────────────────────────────────────────
    QLabel* notesLbl = new QLabel("Notes :");
    notesLbl->setStyleSheet("font-size:11px; color:#888;");
    QTextEdit* notesEdit = new QTextEdit();
    notesEdit->setPlainText(block.notes);
    notesEdit->setFixedHeight(40);
    notesEdit->setPlaceholderText("Notes…");
    vl->addWidget(notesLbl);
    vl->addWidget(notesEdit);

    // ── Connexions live ───────────────────────────────────────────────────
    auto emitUpdate = [this, bid, tradEdit, statusBox, notesEdit]() {
        emit blockUpdated(bid,
                          tradEdit->toPlainText(),
                          statusBox->currentText(),
                          notesEdit->toPlainText());
    };
    connect(tradEdit,  &QTextEdit::textChanged, this, emitUpdate);
    connect(notesEdit, &QTextEdit::textChanged, this, emitUpdate);
    connect(statusBox, &QComboBox::currentTextChanged,
            this, [emitUpdate](const QString&) { emitUpdate(); });

    // Clic sur le frame → sélection sync avec ImageWindow
    connect(idLbl, &QLabel::linkActivated,
            this, [this, bid](const QString&) { emit blockSelected(bid); });
    frame->installEventFilter(this);

    // ── Insertion dans le QListWidget ─────────────────────────────────────
    auto* item = new QListWidgetItem(m_list);
    item->setData(Qt::UserRole, bid);
    item->setSizeHint(frame->sizeHint());
    // Désactive la sélection visuelle bleue de QListWidget
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);

    m_list->addItem(item);
    m_list->setItemWidget(item, frame);
    m_panels[bid] = frame;

    // Recalcule la taille après layout
    frame->adjustSize();
    item->setSizeHint(QSize(frame->sizeHint().width(),
                            frame->sizeHint().height() + 8));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Scroll + surlignage
// ─────────────────────────────────────────────────────────────────────────────

void TextWindow::scrollToBlock(int id)
{
    for (int i = 0; i < m_list->count(); ++i) {
        auto* item = m_list->item(i);
        if (item->data(Qt::UserRole).toInt() == id) {
            // Reset tous
            for (auto* w : m_panels)
                w->setStyleSheet("QFrame { border: 1px solid #444; }");
            // Surligne
            m_panels.value(id)->setStyleSheet(
                "QFrame { border: 2px solid #FFD700; }");
            m_list->scrollToItem(item, QAbstractItemView::PositionAtCenter);
            return;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Réordonnancement par drag
// ─────────────────────────────────────────────────────────────────────────────

void TextWindow::onRowsMoved()
{
    QList<int> newOrder;
    for (int i = 0; i < m_list->count(); ++i) {
        int bid = m_list->item(i)->data(Qt::UserRole).toInt();
        newOrder.append(bid);

        // Met à jour le label ID pour refléter le nouvel ordre
        if (m_panels.contains(bid)) {
            auto* lbl = m_panels[bid]->findChild<QLabel*>();
            // Le label ID est le 2e enfant (après dragHandle)
        }
    }
    emit blocksReordered(newOrder);
}

bool TextWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        auto* w = qobject_cast<QWidget*>(obj);
        if (w && w->property("bubbleId").isValid())
            emit blockSelected(w->property("bubbleId").toInt());
    }
    return QMainWindow::eventFilter(obj, event);
}
