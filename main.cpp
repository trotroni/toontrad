#include <QApplication>
#include <QIcon>
#include "config.h"
#include "windows/main/MainWindow.h"

#include <QDebug>
#include <QtGlobal>


int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("ToonTrad");
    app.setOrganizationName("ToonTrad");
    app.setWindowIcon(QIcon(":/resources/toontrad-icon-red-double-t.svg"));

    Config::load();

    MainWindow window;
    window.show();

    qDebug() << QT_VERSION_STR;

    return app.exec();
}
