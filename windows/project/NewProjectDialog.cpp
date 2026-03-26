#include "NewProjectDialog.h"
#include "ui_NewProjectDialog.h"

#include <QFileDialog>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDebug>

static const QStringList PROJECT_SUBDIRS = {
    "raw",        // images originales téléchargées
    "output",     // exports TXT / JSON traductions
    "renders",    // PNG rendus avec traductions
    "photoshop",  // JSON Photoshop pour plugin PS
};

NewProjectDialog::NewProjectDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::NewProjectDialog)
{
    ui->setupUi(this);

    // Connexions manuelles — pas d'auto-connect via connectSlotsByName
    connect(ui->btnBrowse, &QPushButton::clicked,
            this, &NewProjectDialog::onBrowse);
    connect(ui->editName, &QLineEdit::textChanged,
            this, [this](const QString&) { updatePreview(); });
    connect(ui->editParentFolder, &QLineEdit::textChanged,
            this, [this](const QString&) { updatePreview(); });
    connect(ui->buttonBox, &QDialogButtonBox::accepted,
            this, &NewProjectDialog::onAccept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
}

NewProjectDialog::~NewProjectDialog() { delete ui; }

// ─────────────────────────────────────────────────────────────────────────────

void NewProjectDialog::onBrowse()
{
    qDebug() << "[NewProjectDialog] onBrowse";
    QString path = QFileDialog::getExistingDirectory(
        this,
        "Choisir le dossier parent",
        QDir::homePath(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
    if (!path.isEmpty()) {
        m_parentFolder = path;
        ui->editParentFolder->setText(path);
    }
}

void NewProjectDialog::updatePreview()
{
    QString name   = ui->editName->text().trimmed();
    QString parent = ui->editParentFolder->text().trimmed();

    if (name.isEmpty() || parent.isEmpty()) {
        ui->lblPreview->setText("—");
        return;
    }

    QString base    = parent + "/" + name + "/";
    QString preview = base + "\n";
    preview += "  ├── .ttproject\n";
    for (int i = 0; i < PROJECT_SUBDIRS.size(); ++i) {
        bool last = (i == PROJECT_SUBDIRS.size() - 1);
        preview += QString("  %1── %2/\n")
                       .arg(last ? "└" : "├")
                       .arg(PROJECT_SUBDIRS[i]);
    }
    ui->lblPreview->setText(preview);
}

void NewProjectDialog::onAccept()
{
    ui->lblError->clear();

    QString name   = ui->editName->text().trimmed();
    QString parent = ui->editParentFolder->text().trimmed();

    if (name.isEmpty()) {
        ui->lblError->setText("Le nom du projet est requis.");
        return;
    }
    if (parent.isEmpty() || !QDir(parent).exists()) {
        ui->lblError->setText("Dossier parent invalide ou introuvable.");
        return;
    }

    QString projectPath = QDir(parent).absoluteFilePath(name);

    if (QDir(projectPath).exists()) {
        auto btn = QMessageBox::question(
            this, "Dossier existant",
            "Le dossier " + projectPath + " existe déjà.\nL'utiliser quand même ?");
        if (btn != QMessageBox::Yes) return;
    }

    if (!createProjectStructure(projectPath, name)) {
        ui->lblError->setText("Impossible de créer le projet dans : " + projectPath);
        return;
    }

    m_parentFolder = projectPath;
    accept();
}

bool NewProjectDialog::createProjectStructure(const QString& path,
                                               const QString& name)
{
    QDir dir;

    if (!dir.mkpath(path)) {
        qWarning() << "Impossible de créer:" << path;
        return false;
    }

    for (const QString& sub : PROJECT_SUBDIRS) {
        if (!dir.mkpath(path + "/" + sub))
            qWarning() << "Impossible de créer sous-dossier:" << path + "/" + sub;
    }

    // Crée project.ttproject (JSON)
    QJsonObject root;
    root["name"]    = name;
    root["version"] = 2;
    root["pages"]   = QJsonArray();

    QFile f(path + "/.ttproject");
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        f.close();
        qDebug() << "[NewProjectDialog] project.ttproject créé →" << path;
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────

QString NewProjectDialog::projectName() const
{
    return ui->editName->text().trimmed();
}

QString NewProjectDialog::projectPath() const
{
    return m_parentFolder;
}
