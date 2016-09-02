#ifndef APPGUI_H
#define APPGUI_H

#include <QMainWindow>
#include <QObject>
#include <QWidget>
#include <QSystemTrayIcon>

#include "Common.h"
#include "MainWindow.h"

class AppGui : public QApplication
{
public:
    AppGui(int &argc, char **argv);
    virtual ~AppGui();
    void mainWindowShow();
    void mainWindowHide();
    void enableDameon();
    void disableDaemon();

private:
     MainWindow win;
     QSystemTrayIcon * systray;
};

#endif // APPGUI_H
