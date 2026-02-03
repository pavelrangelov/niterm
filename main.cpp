#include <QApplication>
#include <QStyleFactory>

#include "mainwindow.h"
#include "settings.h"

///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QApplication::setStyle(QStyleFactory::create("Fusion"));

    app.setOrganizationName(ORG_NAME);
    app.setApplicationName(APP_NAME);

    MainWindow win;
    win.show();

    return app.exec();
}
