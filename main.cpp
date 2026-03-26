#include <QApplication>
#include "config.h"
#include "core/ProjectManager.h"
#include "windows/main/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(true);
    app.setApplicationName("ToonTrad");
    app.setOrganizationName("ToonTrad");
    app.setApplicationVersion(Config::VERSION_STR);

    Config::load();
    ProjectManager::instance().load();

    // argv[1] peut être un chemin vers un .ttproject
    // (double-clic depuis l'Explorateur Windows ou le Finder hors bundle).
    // Sur macOS avec bundle, QFileOpenEvent prend le relais dans MainWindow::event().
    QString openPath;
    if (argc > 1)
        openPath = QString::fromLocal8Bit(argv[1]);

    MainWindow window(openPath);
    window.show();

    //qDebug() << QT_VERSION_STR;

    return app.exec();
}
