#include <QApplication>
#include "config.h"
#include "windows/main/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("ToonTrad");
    app.setOrganizationName("ToonTrad");

    Config::load();

    MainWindow window;
    window.show();

    return app.exec();
}