#include "SettingsWindow.h"
#include "ui_SettingsWindow.h"
#include "../../config.h"
#include "../../core/models.h"
#include "../../core/OCRManager.h"
#include <QFileDialog>
#include <QMessageBox>

SettingsWindow::SettingsWindow(const OCRConfig& config, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::SettingsWindow)
    , m_config(config)
{
    ui->setupUi(this);
    setModal(true);

    auto langs        = ToonTrad::languageList();
    auto langsDisplay = ToonTrad::languageDisplayNames();
    for (int i = 0; i < langs.size(); ++i)
        ui->comboLanguage->addItem(langsDisplay[i], langs[i]);
    int li = langs.indexOf(config.language);
    if (li >= 0) ui->comboLanguage->setCurrentIndex(li);

    const QList<int> psmVals = {3, 6, 11, 13};
    ui->comboPSM->setCurrentIndex(psmVals.indexOf(config.psmMode));

    ui->sliderConfidence->setValue(static_cast<int>(config.confidenceThreshold * 100));
    ui->lblConfValue->setText(QString::number(ui->sliderConfidence->value()) + "%");
    ui->spinMinArea->setValue(config.minBubbleArea);

    ui->editPythonBin->setText(Config::pythonBin);
    ui->editPythonScript->setText(Config::pythonScript);

    connect(ui->sliderConfidence, &QSlider::valueChanged,
            this, &SettingsWindow::onConfidenceChanged);
    connect(ui->spinMinArea, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SettingsWindow::onMinAreaChanged);

    connect(ui->btnBrowsePython, &QPushButton::clicked, this, [this]() {
        QString p = QFileDialog::getOpenFileName(this, "Sélectionner Python");
        if (!p.isEmpty()) ui->editPythonBin->setText(p);
    });

    connect(ui->btnBrowseScript, &QPushButton::clicked, this, [this]() {
        QString p = QFileDialog::getOpenFileName(this, "Sélectionner main_ocr.py",
                                                  "", "Python (*.py)");
        if (!p.isEmpty()) ui->editPythonScript->setText(p);
    });

    connect(ui->btnTestPython, &QPushButton::clicked, this, [this]() {
        Config::pythonBin    = ui->editPythonBin->text();
        Config::pythonScript = ui->editPythonScript->text();
        QString err;
        if (OCRManager::checkPythonAvailable(&err))
            QMessageBox::information(this, "Python OK", "Python et le script sont accessibles.");
        else
            QMessageBox::critical(this, "Erreur Python", err);
    });

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

SettingsWindow::~SettingsWindow() { delete ui; }

void SettingsWindow::onConfidenceChanged(int value)
{
    ui->lblConfValue->setText(QString::number(value) + "%");
    m_config.confidenceThreshold = value / 100.0;
}

void SettingsWindow::onMinAreaChanged(int value)
{
    m_config.minBubbleArea = value;
}

OCRConfig SettingsWindow::config() const
{
    OCRConfig c = m_config;

    int li = ui->comboLanguage->currentIndex();
    c.language = ToonTrad::languageList().value(li, "en");

    const QList<int> psmVals = {3, 6, 11, 13};
    int pi = ui->comboPSM->currentIndex();
    c.psmMode = psmVals.value(pi, 6);

    Config::pythonBin    = ui->editPythonBin->text();
    Config::pythonScript = ui->editPythonScript->text();
    Config::save();

    return c;
}
