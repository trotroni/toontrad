#include "OCRwindow.h"
#include "ui_OCRwindow.h"

#include <QFrame>
#include <QVBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QFileDialog>
#include <QImage>
#include "../../core/OCRManager.h"


OCRwindow::OCRwindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::OCRwindow)
{
    ui->setupUi(this);

    // Exemple
    addOCRBlock(1, "Hello");
    addOCRBlock(2, "How are you");
}

OCRwindow::~OCRwindow()
{
    delete ui;
}

void OCRwindow::addOCRBlock(int id, QString original)
{
    QFrame *block = new QFrame();
    block->setFrameShape(QFrame::Box);

    QVBoxLayout *layout = new QVBoxLayout(block);

    QLabel *label = new QLabel("ID : " + QString::number(id));

    QTextEdit *orig = new QTextEdit();
    orig->setText(original);

    QTextEdit *trad = new QTextEdit();
    trad->setPlaceholderText("Traduction...");

    layout->addWidget(label);
    layout->addWidget(orig);
    layout->addWidget(trad);

    ui->ocrLayout->addWidget(block);
}

void OCRwindow::loadTestImage()
{
    QString file = QFileDialog::getOpenFileName(
        this,
        "Open Image",
        "",
        "Images (*.png *.jpg *.jpeg)"
    );

    if(file.isEmpty())
        return;

    QImage image(file);

    OCRManager ocr;
    auto blocks = ocr.extractText(image);

    for(const auto& block : blocks)
    {
        addOCRBlock(block.boundingBox, block.originalText);
    }
}