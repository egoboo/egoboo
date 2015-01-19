#include "controllerwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("Egoboo");
    QCoreApplication::setOrganizationDomain("egoboo.sourceforge.net");
    QCoreApplication::setApplicationName("EOV-qt");
    QCoreApplication::setApplicationVersion("0.1");

    QApplication a(argc, argv);
    ControllerWindow w;
    w.show();

    return a.exec();
}
