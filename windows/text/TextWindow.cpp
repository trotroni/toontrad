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
#include <QEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QDebug>

// ─────────────────────────────────────────────────────────────────────────────
//  Constructeur
// ─────────────────────────────────────────────────────────────────────────────

TextWindow::TextWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::TextWindow)
{
    ui->setupUi(this);

    m_list = new QListWidget(ui->centralwidget);
    m_list->setDragDropMode(QAbstractItemView::InternalMove);
    m_list->setDefaultDropAction(Qt::MoveAction);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setSpacing(2);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    auto* rootLayout = qobject_cast<QVBoxLayout*>(ui->centralwidget->layout());
    QLayoutItem* old = rootLayout->takeAt(1);
    if (old) { if (old->widget()) old->widget()->hide(); delete old; }
    rootLayout->insertWidget(1, m_list);

    connect(m_list->model(), &QAbstractItemModel::rowsMoved,
            this, &TextWindow::onRowsMoved);
}

TextWindow::~TextWindow() { delete ui; }

// ─────────────────────────────────────────────────────────────────────────────
//  API publique
// ─────────────────────────────────────────────────────────────────────────────

void TextWindow::clearPanels()
{
    m_list->clear();
    m_blockData.clear();
    m_order.clear();
}

void TextWindow::setBlocks(const std::vector<TextBlock>& blocks)
{
    clearPanels();

    for (const auto& b : blocks) {
        m_blockData[b.id] = b;
        m_order.append(b.id);
    }

    for (int bid : m_order) {
        auto* item = new QListWidgetItem(m_list);
        item->setData(Qt::UserRole, bid);
        item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
        m_list->addItem(item);
    }
    rebuildAllWidgets();

    ui->lblPage->setText(QString("%1 bulle(s)").arg(blocks.size()));
}

void TextWindow::scrollToBlock(int id)
{
    for (int i = 0; i < m_list->count(); ++i) {
        auto* item = m_list->item(i);
        if (item->data(Qt::UserRole).toInt() != id) continue;

        for (int j = 0; j < m_list->count(); ++j)
            if (auto* w = m_list->itemWidget(m_list->item(j)))
                w->setStyleSheet("QFrame { border: 1px solid #444; }");

        if (auto* w = m_list->itemWidget(item))
            w->setStyleSheet("QFrame { border: 2px solid #FFD700; }");

        m_list->scrollToItem(item, QAbstractItemView::PositionAtCenter);
        return;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Construction d'un widget de bulle
// ─────────────────────────────────────────────────────────────────────────────

void TextWindow::buildWidget(int bid)
{
    QListWidgetItem* targetItem = nullptr;
    for (int i = 0; i < m_list->count(); ++i) {
        if (m_list->item(i)->data(Qt::UserRole).toInt() == bid) {
            targetItem = m_list->item(i);
            break;
        }
    }
    if (!targetItem || !m_blockData.contains(bid)) return;

    const TextBlock& block = m_blockData[bid];

    QFrame* frame = new QFrame();
    frame->setFrameShape(QFrame::Box);
    frame->setProperty("bubbleId", bid);

    QVBoxLayout* vl = new QVBoxLayout(frame);
    vl->setContentsMargins(6, 6, 6, 6);
    vl->setSpacing(4);

    // ── En-tête ───────────────────────────────────────────────────────────
    QHBoxLayout* hl = new QHBoxLayout();

    // Poignée de drag — intercepte les événements souris pour le drag manuel
    QLabel* dragHandle = new QLabel("⠿");
    dragHandle->setStyleSheet("color:#aaa; font-size:18px; padding:0 6px;");
    dragHandle->setToolTip("Glisser pour réordonner");
    dragHandle->setCursor(Qt::SizeVerCursor);
    dragHandle->setProperty("isDragHandle", true);
    dragHandle->setProperty("bubbleId", bid);
    dragHandle->installEventFilter(this);

    QLabel* idLbl = new QLabel(
        QString("<b>Bulle #%1</b> &nbsp;"
                "<span style='color:#888; font-size:10px;'>"
                "conf:%2% &nbsp;|&nbsp; %3,%4 &nbsp;%5×%6</span>")
            .arg(bid)
            .arg(static_cast<int>(block.confidence * 100))
            .arg(block.boundingBox.x()).arg(block.boundingBox.y())
            .arg(block.boundingBox.width()).arg(block.boundingBox.height()));
    idLbl->setTextFormat(Qt::RichText);
    idLbl->setProperty("bubbleId", bid);
    idLbl->installEventFilter(this);

    hl->addWidget(dragHandle);
    hl->addWidget(idLbl);
    hl->addStretch();
    vl->addLayout(hl);

    // ── RAW (éditable) ────────────────────────────────────────────────────
    QLabel* rawLbl = new QLabel("RAW :");
    rawLbl->setStyleSheet("font-size:11px; color:#888;");
    QTextEdit* rawEdit = new QTextEdit();
    rawEdit->setPlainText(block.originalText);
    rawEdit->setFixedHeight(55);
    rawEdit->setPlaceholderText("Texte OCR brut — modifiable…");
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
    auto emitUpdate = [this, bid, rawEdit, tradEdit, statusBox, notesEdit]() {
        if (m_blockData.contains(bid)) {
            m_blockData[bid].originalText   = rawEdit->toPlainText();
            m_blockData[bid].translatedText = tradEdit->toPlainText();
            m_blockData[bid].status         = statusBox->currentText();
            m_blockData[bid].notes          = notesEdit->toPlainText();
        }
        emit blockUpdated(bid,
                          tradEdit->toPlainText(),
                          statusBox->currentText(),
                          notesEdit->toPlainText());
    };
    connect(rawEdit,   &QTextEdit::textChanged, this, emitUpdate);
    connect(tradEdit,  &QTextEdit::textChanged, this, emitUpdate);
    connect(notesEdit, &QTextEdit::textChanged, this, emitUpdate);
    connect(statusBox, &QComboBox::currentTextChanged,
            this, [emitUpdate](const QString&) { emitUpdate(); });

    frame->adjustSize();
    targetItem->setSizeHint(QSize(frame->sizeHint().width(),
                                   frame->sizeHint().height() + 8));
    m_list->setItemWidget(targetItem, frame);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Reconstruction de tous les widgets après drag
// ─────────────────────────────────────────────────────────────────────────────

void TextWindow::rebuildAllWidgets()
{
    for (int i = 0; i < m_list->count(); ++i) {
        int bid = m_list->item(i)->data(Qt::UserRole).toInt();
        buildWidget(bid);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Slot rowsMoved
// ─────────────────────────────────────────────────────────────────────────────

void TextWindow::onRowsMoved(const QModelIndex&, int, int, const QModelIndex&, int)
{
    m_order.clear();
    for (int i = 0; i < m_list->count(); ++i)
        m_order.append(m_list->item(i)->data(Qt::UserRole).toInt());

    rebuildAllWidgets();
    emit blocksReordered(m_order);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Event filter
//
//  - Clic sur idLbl  → blockSelected
//  - Drag depuis dragHandle → relaye les événements souris au viewport
//    du QListWidget pour que le drag natif se déclenche
// ─────────────────────────────────────────────────────────────────────────────

bool TextWindow::eventFilter(QObject* obj, QEvent* event)
{
    auto* w = qobject_cast<QWidget*>(obj);
    if (!w) return false;

    const bool isDragHandle = w->property("isDragHandle").toBool();
    const bool hasBubbleId  = w->property("bubbleId").isValid();

    if (event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);

        // Clic → sélection de la bulle
        if (hasBubbleId)
            emit blockSelected(w->property("bubbleId").toInt());

        if (isDragHandle && me->button() == Qt::LeftButton) {
            // Mémorise la position de départ pour le drag
            m_dragInProgress = true;
            m_dragStartPos   = me->globalPosition().toPoint();

            // Trouve la ligne dans la liste correspondant à cette poignée
            int bid = w->property("bubbleId").toInt();
            m_dragSourceRow = -1;
            for (int i = 0; i < m_list->count(); ++i) {
                if (m_list->item(i)->data(Qt::UserRole).toInt() == bid) {
                    m_dragSourceRow = i;
                    break;
                }
            }

            // Sélectionne l'item dans la liste (nécessaire pour le drag Qt)
            if (m_dragSourceRow >= 0)
                m_list->setCurrentRow(m_dragSourceRow);

            // Relaie le MouseButtonPress au viewport pour initier le drag
            QPoint viewPos = m_list->viewport()->mapFromGlobal(
                me->globalPosition().toPoint());
            QMouseEvent relayed(QEvent::MouseButtonPress,
                                viewPos,
                                me->globalPosition().toPoint(),
                                me->button(), me->buttons(), me->modifiers());
            QApplication::sendEvent(m_list->viewport(), &relayed);
            return true;  // consommé — on gère nous-mêmes
        }
    }

    if (event->type() == QEvent::MouseMove && m_dragInProgress) {
        auto* me = static_cast<QMouseEvent*>(event);
        QPoint viewPos = m_list->viewport()->mapFromGlobal(
            me->globalPosition().toPoint());
        QMouseEvent relayed(QEvent::MouseMove,
                            viewPos,
                            me->globalPosition().toPoint(),
                            me->button(), me->buttons(), me->modifiers());
        QApplication::sendEvent(m_list->viewport(), &relayed);
        return true;
    }

    if (event->type() == QEvent::MouseButtonRelease && m_dragInProgress) {
        m_dragInProgress = false;
        m_dragSourceRow  = -1;
        auto* me = static_cast<QMouseEvent*>(event);
        QPoint viewPos = m_list->viewport()->mapFromGlobal(
            me->globalPosition().toPoint());
        QMouseEvent relayed(QEvent::MouseButtonRelease,
                            viewPos,
                            me->globalPosition().toPoint(),
                            me->button(), me->buttons(), me->modifiers());
        QApplication::sendEvent(m_list->viewport(), &relayed);
        return true;
    }

    return false;
}