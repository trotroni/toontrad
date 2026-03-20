#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui { class NewProjectDialog; }
QT_END_NAMESPACE

class NewProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget* parent = nullptr);
    ~NewProjectDialog();

    // Résultats après accept()
    QString projectName()   const;
    QString projectPath()   const;  // chemin complet = parent/name

private slots:
    void onBrowse();
    void onAccept();

private:
    Ui::NewProjectDialog* ui;
    QString m_parentFolder;

    void updatePreview();
    bool createProjectStructure(const QString& path, const QString& name);
};

#endif // NEWPROJECTDIALOG_H
