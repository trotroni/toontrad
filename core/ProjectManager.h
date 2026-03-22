#ifndef PROJECTMANAGER_H
#define PROJECTMANAGER_H

#include <QString>
#include <QStringList>
#include <QList>
#include <vector>
#include "TextBlock.h"

// Extension officielle des fichiers projet ToonTrad
inline constexpr const char* TTPROJECT_EXT      = "ttproject";
inline constexpr const char* TTPROJECT_FILENAME  = "project.ttproject";
// Ancien nom — utilisé uniquement pour la migration automatique
inline constexpr const char* TTPROJECT_LEGACY    = "project.json";


struct ImagePage
{
    QString relativePath;
    bool    ocrDone = false;
    std::vector<TextBlock> blocks;
};


class Project
{
public:
    QString          name;
    QString          rootPath;
    QList<ImagePage> pages;

    void scanImages();

    bool load();
    bool save() const;

    QString    absolutePath(const ImagePage& page) const;
    ImagePage* findPage(const QString& relativePath);

private:
    static const QStringList IMAGE_EXTENSIONS;
};


class ProjectManager
{
public:
    static ProjectManager& instance();

    void load();
    void save() const;

    Project* addProject(const QString& rootPath);
    void     removeProject(const QString& rootPath);

    QList<Project>& projects() { return m_projects; }

private:
    ProjectManager() = default;
    QList<Project> m_projects;
};

#endif // PROJECTMANAGER_H
