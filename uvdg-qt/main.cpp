
#include <Windows.h>
#include <QtCore/QtCore>
#include <QtGui/QtGui>
#include "RtlUvdParser.h"
#include "MainWindow.h"

#pragma comment(lib, "QtCore4.lib")
#pragma comment(lib, "QtGui4.lib")
#pragma comment(lib, "QtNetwork4.lib")

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpcmdline, int ncmdshow)
{
    QApplication app(ncmdshow, (char **)lpcmdline);
    MainWindow *mainWindow = new MainWindow();
    mainWindow->show();

    return app.exec();
}
