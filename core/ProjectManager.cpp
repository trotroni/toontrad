#include "ProjectManager.h"
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QSettings>
#include <QDebug>

const QStringList Project::IMAGE_EXTENSIONS = {
    "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.webp", "*.tiff", "*.tif"
};

QString Project::absolutePath(const ImagePage& page) const
{
    return QDir(rootPath).absoluteFilePath(page.relativePath);
}

ImagePage* Project::findPage(const QString& relativePath)
{
    for (auto& p : pages)
        if (p.relativePath == relativePath) return &p;
    return nullptr;
}

void Project::scanImages()
{
    QDir root(rootPath);
    if (!root.exists()) return;

    QMap<QString, ImagePage> existing;
    for (const auto& p : pages)
        existing[p.relativePath] = p;

    pages.clear();

    QDirIterator it(rootPath, IMAGE_EXTENSIONS,
                    QDir::Files, QDirIterator::Subdirectories);
    QStringList found;
    while (it.hasNext()) {
        it.next();
        found << root.relativeFilePath(it.filePath());
    }
    found.sort();

    for (const QString& rel : found) {
        if (existing.contains(rel))
            pages.append(existing[rel]);
        else {
            ImagePage page;
            page.relativePath = rel;
            pages.append(page);
        }
    }
    qDebug() << "Project::scanImages:" << pages.size() << "images dans" << rootPath;
}

bool Project::load()
{
    QString jsonPath = QDir(rootPath).filePath("project.json");
    QFile file(jsonPath);
    if (!file.exists()) {
        scanImages();
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonObject root = doc.object();
    name = root["name"].toString(QFileInfo(rootPath).fileName());

    pages.clear();
    for (const QJsonValue& v : root["pages"].toArray()) {
        QJsonObject po = v.toObject();
        ImagePage page;
        page.relativePath = po["file"].toString();
        page.ocrDone      = po["ocrDone"].toBool(false);
        for (const QJsonValue& bv : po["blocks"].toArray())
            page.blocks.push_back(TextBlock::fromJson(bv.toObject()));
        pages.append(page);
    }

    scanImages();
    return true;
}

bool Project::save() const
{
    QDir dir(rootPath);
    if (!dir.exists()) return false;

    QJsonObject root;
    root["name"]    = name;
    root["version"] = 2;

    QJsonArray pagesArr;
    for (const auto& page : pages) {
        QJsonObject po;
        po["file"]    = page.relativePath;
        po["ocrDone"] = page.ocrDone;
        QJsonArray blocks;
        for (const auto& b : page.blocks)
            blocks.append(b.toJson());
        po["blocks"] = blocks;
        pagesArr.append(po);
    }
    root["pages"] = pagesArr;

    QFile file(dir.filePath("project.json"));
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    qDebug() << "Project::save →" << dir.filePath("project.json");
    return true;
}

ProjectManager& ProjectManager::instance()
{
    static ProjectManager inst;
    return inst;
}

void ProjectManager::load()
{
    QSettings s("ToonTrad", "ToonTrad");
    QStringList paths = s.value("projectPaths").toStringList();
    m_projects.clear();
    for (const QString& path : paths) {
        if (!QDir(path).exists()) continue;
        Project p;
        p.rootPath = path;
        p.name     = QFileInfo(path).fileName();
        p.load();
        m_projects.append(p);
    }
}

void ProjectManager::save() const
{
    QSettings s("ToonTrad", "ToonTrad");
    QStringList paths;
    for (const auto& p : m_projects)
        paths << p.rootPath;
    s.setValue("projectPaths", paths);
}

Project* ProjectManager::addProject(const QString& rootPath)
{
    for (auto& p : m_projects)
        if (p.rootPath == rootPath) return &p;

    Project p;
    p.rootPath = rootPath;
    p.name     = QFileInfo(rootPath).fileName();
    p.load();
    m_projects.append(p);
    save();
    return &m_projects.last();
}

void ProjectManager::removeProject(const QString& rootPath)
{
    m_projects.removeIf([&](const Project& p) { return p.rootPath == rootPath; });
    save();
}
