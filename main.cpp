#include <QApplication>
#include "config.h"
#include "windows/main/mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("ToonTrad - Debug");
    app.setOrganizationName("ToonTrad");

    Config::load();

    MainWindow window;
    window.show();

    return app.exec();
}
