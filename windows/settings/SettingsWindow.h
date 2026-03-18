#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QDialog>
#include "../../core/OCRConfig.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SettingsWindow; }
QT_END_NAMESPACE

class SettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsWindow(const OCRConfig& config, QWidget* parent = nullptr);
    ~SettingsWindow();

    OCRConfig config() const;

private slots:
    void onConfidenceChanged(int value);
    void onMinAreaChanged(int value);

private:
    Ui::SettingsWindow* ui;
    OCRConfig m_config;
};

#endif // SETTINGSWINDOW_H
