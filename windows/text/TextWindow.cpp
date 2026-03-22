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

    // Remplace le QScrollArea par le QListWidget dans le layout
    auto* rootLayout = qobject_cast<QVBoxLayout*>(ui->centralwidget->layout());
    QLayoutItem* old = rootLayout->takeAt(1);
    if (old) { if (old->widget()) old->widget()->hide(); delete old; }
    rootLayout->insertWidget(1, m_list);

    // ── Connexion rowsMoved avec la signature correcte ────────────────────
    // Qt envoie rowsMoved APRÈS le déplacement interne — c'est le bon moment
    // pour re-assigner les setItemWidget (qui ne suivent pas le drag).
    connect(m_list->model(), &QAbstractItemModel::rowsMoved,
            this, &TextWindow::onRowsMoved);
}

TextWindow::~TextWindow() { delete ui; }

// ─────────────────────────────────────────────────────────────────────────────
//  API publique
// ─────────────────────────────────────────────────────────────────────────────

void TextWindow::clearPanels()
{
    m_list->clear();       // supprime items ET libère les widgets setItemWidget
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

    // Crée les items vides dans le bon ordre, puis assigne les widgets
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

        // Reset highlight de tous les frames
        for (int j = 0; j < m_list->count(); ++j) {
            if (auto* w = m_list->itemWidget(m_list->item(j)))
                w->setStyleSheet("QFrame { border: 1px solid #444; }");
        }
        // Surligne le frame ciblé
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
    // Trouve l'item correspondant dans la liste
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

    QLabel* dragHandle = new QLabel("⠿");
    dragHandle->setStyleSheet("color:#555; font-size:16px; padding:0 4px;");
    dragHandle->setToolTip("Glisser pour réordonner");

    QLabel* idLbl = new QLabel(
        QString("<b>Bulle #%1</b> &nbsp;"
                "<span style='color:#888; font-size:10px;'>"
                "conf:%2% &nbsp;|&nbsp; %3,%4 &nbsp;%5×%6</span>")
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
    rawEdit->setReadOnly(true);
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

    // ── Connexions live → sauvegarde dans m_blockData ─────────────────────
    auto emitUpdate = [this, bid, tradEdit, statusBox, notesEdit]() {
        // Met à jour le cache local
        if (m_blockData.contains(bid)) {
            m_blockData[bid].translatedText = tradEdit->toPlainText();
            m_blockData[bid].status         = statusBox->currentText();
            m_blockData[bid].notes          = notesEdit->toPlainText();
        }
        emit blockUpdated(bid,
                          tradEdit->toPlainText(),
                          statusBox->currentText(),
                          notesEdit->toPlainText());
    };
    connect(tradEdit,  &QTextEdit::textChanged, this, emitUpdate);
    connect(notesEdit, &QTextEdit::textChanged, this, emitUpdate);
    connect(statusBox, &QComboBox::currentTextChanged,
            this, [emitUpdate](const QString&) { emitUpdate(); });

    // Clic sur le panneau → blockSelected
    frame->installEventFilter(this);

    // ── Affectation à l'item ──────────────────────────────────────────────
    frame->adjustSize();
    targetItem->setSizeHint(QSize(frame->sizeHint().width(),
                                   frame->sizeHint().height() + 8));
    m_list->setItemWidget(targetItem, frame);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Reconstruction de TOUS les widgets (nécessaire après un drag)
//
//  Explication du bug Qt corrigé ici :
//  QListWidget::setItemWidget stocke widget↔item par POINTEUR d'item.
//  Lors d'un drag interne, les QListWidgetItem* sont déplacés dans le modèle
//  mais les widgets (qui sont des enfants de QListWidget viewport) restent
//  associés à leur position initiale. Résultat : le widget affiché ne
//  correspond plus à l'item après le drag.
//  Solution : après rowsMoved, on relit l'ordre réel des items et on
//  réassigne setItemWidget pour chaque item dans le bon ordre.
// ─────────────────────────────────────────────────────────────────────────────

void TextWindow::rebuildAllWidgets()
{
    // Reconstruit dans l'ordre actuel des items
    for (int i = 0; i < m_list->count(); ++i) {
        int bid = m_list->item(i)->data(Qt::UserRole).toInt();
        buildWidget(bid);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Slot rowsMoved — déclenché APRÈS le drag interne
// ─────────────────────────────────────────────────────────────────────────────

void TextWindow::onRowsMoved(const QModelIndex& /*parent*/,
                              int /*start*/, int /*end*/,
                              const QModelIndex& /*destination*/,
                              int /*row*/)
{
    // 1. Relit le nouvel ordre des IDs depuis le modèle
    m_order.clear();
    for (int i = 0; i < m_list->count(); ++i)
        m_order.append(m_list->item(i)->data(Qt::UserRole).toInt());

    // 2. Reconstruit les widgets dans le bon ordre
    //    (corrige le désalignement item↔widget causé par le drag Qt)
    rebuildAllWidgets();

    // 3. Notifie l'ImageWindow et le ProjectManager du nouvel ordre
    emit blocksReordered(m_order);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Event filter — clic sur panneau → blockSelected
// ─────────────────────────────────────────────────────────────────────────────

bool TextWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        auto* w = qobject_cast<QWidget*>(obj);
        if (w && w->property("bubbleId").isValid())
            emit blockSelected(w->property("bubbleId").toInt());
    }
    return QMainWindow::eventFilter(obj, event);
}
