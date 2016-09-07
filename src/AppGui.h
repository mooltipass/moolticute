#ifndef APPGUI_H
#define APPGUI_H

#include <QMainWindow>
#include <QObject>
#include <QWidget>
#include <QSystemTrayIcon>

#include "Common.h"
#include "MainWindow.h"
#include "WSClient.h"
#include "DaemonMenuAction.h"

class AppGui : public QApplication
{
    Q_OBJECT
public:
    AppGui(int &argc, char **argv);
    virtual ~AppGui();

    bool initialize();

    void mainWindowShow();
    void mainWindowHide();
    void enableDaemon();
    void disableDaemon();

private slots:
    void connectedChanged();

private:
     MainWindow *win;
     QSystemTrayIcon * systray;
     WSClient *wsClient;
     QAction* showConfigApp;
     DaemonMenuAction *daemonAction;

     QProcess *daemonProcess;
     bool dRunning = false;
     bool needRestart = false;
     bool aboutToQuit = false;

     quint64 lastStateChange = 0;
};

#endif // APPGUI_H
