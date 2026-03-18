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

    // Langue
    auto langs     = ToonTrad::languageList();
    auto langsDisp = ToonTrad::languageDisplayNames();
    for (int i = 0; i < langs.size(); ++i)
        ui->comboLanguage->addItem(langsDisp[i], langs[i]);
    int li = langs.indexOf(config.language);
    if (li >= 0) ui->comboLanguage->setCurrentIndex(li);

    // PSM
    const QList<int> psmVals = {3, 6, 11, 13};
    ui->comboPSM->setCurrentIndex(psmVals.indexOf(config.psmMode));

    // Confidence
    ui->sliderConfidence->setValue(static_cast<int>(config.confidenceThreshold * 100));
    ui->lblConfValue->setText(QString::number(ui->sliderConfidence->value()) + "%");

    // Min area
    ui->spinMinArea->setValue(config.minBubbleArea);

    // Inner ratio
    ui->sliderInnerRatio->setValue(static_cast<int>(Config::innerRectRatio * 100));
    ui->lblInnerRatio->setText(QString::number(ui->sliderInnerRatio->value()) + "%");

    // VRAM / RAM
    ui->sliderVRAM->setValue(static_cast<int>(config.gpuMemFraction * 100));
    ui->lblVRAMVal->setText(QString::number(ui->sliderVRAM->value()) + "%");
    ui->sliderRAM->setValue(4);
    ui->lblRAMVal->setText("4 GB");

    // Python
    ui->editPythonBin->setText(Config::pythonBin);
    ui->editDetectScript->setText(Config::detectScript);

    // Sliders
    connect(ui->sliderConfidence, &QSlider::valueChanged, this, &SettingsWindow::onConfidenceChanged);
    connect(ui->spinMinArea, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsWindow::onMinAreaChanged);
    connect(ui->sliderInnerRatio, &QSlider::valueChanged, this, [this](int v) {
        ui->lblInnerRatio->setText(QString::number(v) + "%");
    });
    connect(ui->sliderVRAM, &QSlider::valueChanged, this, [this](int v) {
        ui->lblVRAMVal->setText(QString::number(v) + "%");
    });
    connect(ui->sliderRAM, &QSlider::valueChanged, this, [this](int v) {
        ui->lblRAMVal->setText(QString::number(v) + " GB");
    });

    // Browse
    connect(ui->btnBrowsePython, &QPushButton::clicked, this, [this]() {
        QString p = QFileDialog::getOpenFileName(this, "Sélectionner Python");
        if (!p.isEmpty()) ui->editPythonBin->setText(p);
    });
    connect(ui->btnBrowseScript, &QPushButton::clicked, this, [this]() {
        QString p = QFileDialog::getOpenFileName(this, "Sélectionner detect.py", "", "Python (*.py)");
        if (!p.isEmpty()) ui->editDetectScript->setText(p);
    });

    // Test
    connect(ui->btnTestPython, &QPushButton::clicked, this, [this]() {
        Config::pythonBin    = ui->editPythonBin->text();
        Config::detectScript = ui->editDetectScript->text();
        QString err;
        if (OCRManager::checkPythonAvailable(&err))
            QMessageBox::information(this, "OK", "Python et detect.py sont accessibles.");
        else
            QMessageBox::critical(this, "Erreur", err);
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

    auto langs = ToonTrad::languageList();
    c.language = langs.value(ui->comboLanguage->currentIndex(), "en");

    const QList<int> psmVals = {3, 6, 11, 13};
    c.psmMode = psmVals.value(ui->comboPSM->currentIndex(), 6);

    c.gpuMemFraction = ui->sliderVRAM->value() / 100.0;
    c.ramFraction    = ui->sliderRAM->value() / 64.0;

    Config::innerRectRatio = ui->sliderInnerRatio->value() / 100.0;
    Config::pythonBin      = ui->editPythonBin->text();
    Config::detectScript   = ui->editDetectScript->text();
    Config::save();

    return c;
}
