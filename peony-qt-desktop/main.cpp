#include "desktop-window.h"
#include "peony-desktop-application.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    PeonyDesktopApplication a(argc, argv);
    if (a.isSecondary())
        return 0;

    return a.exec();
}
