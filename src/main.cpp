#include "constants.h"
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication::setApplicationName(Constants::appName);
    QApplication::setApplicationVersion(Constants::appVersion);
    QApplication::setApplicationDisplayName("Argon2 UI");
    QApplication::setOrganizationName(Constants::orgName);
    QApplication::setOrganizationDomain(Constants::orgDomain);

    QApplication application(argc, argv);

    MainWindow window;
    window.show();

    return application.exec();
}
