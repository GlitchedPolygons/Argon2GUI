#include "constants.h"
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication::setApplicationName(Constants::appName);
    QApplication::setApplicationVersion(Constants::appVersion);
    QApplication::setApplicationDisplayName(Constants::appName);
    QApplication::setOrganizationName(Constants::orgName);
    QApplication::setOrganizationDomain(Constants::orgDomain);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
