#include "constants.h"
#include "mainwindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication::setApplicationName(Constants::appName);
    QApplication::setApplicationVersion(Constants::appVersion);
    QApplication::setApplicationDisplayName("Argon2 GUI");
    QApplication::setOrganizationName(Constants::orgName);
    QApplication::setOrganizationDomain(Constants::orgDomain);

    QApplication application(argc, argv);
    application.setWindowIcon(QIcon(":/img/icon.png"));

    MainWindow window;

    QObject::connect(&application, SIGNAL(focusChanged(QWidget*,QWidget*)), &window, SLOT(onChangedFocus(QWidget*,QWidget*)));

    window.show();

    return application.exec();
}
