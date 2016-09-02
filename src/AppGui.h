#ifndef APPGUI_H
#define APPGUI_H

#include <QMainWindow>
#include <QObject>
#include <QWidget>
#include <QSystemTrayIcon>

#include "Common.h"
#include "MainWindow.h"
#include "WSClient.h"

class AppGui : public QApplication
{
public:
    AppGui(int &argc, char **argv);
    virtual ~AppGui();
    void mainWindowShow();
    void mainWindowHide();
    void enableDameon();
    void disableDaemon();
private slots:
    void connectedChanged();
private:
     MainWindow *win;
     QSystemTrayIcon * systray;
     WSClient *wsClient;
     QAction* showConfigApp;
};

#endif // APPGUI_H
