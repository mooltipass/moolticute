#ifndef APPGUI_H
#define APPGUI_H

#include <QMainWindow>
#include <QObject>
#include <QWidget>
#include <QSystemTrayIcon>
#include <QLocalServer>
#include <QLocalSocket>

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
    void searchDaemonTick();
    void slotConnectionEstablished();

    void daemonLogRead();

private:
     MainWindow *win = nullptr;
     QSystemTrayIcon *systray = nullptr;
     WSClient *wsClient = nullptr;
     QAction *showConfigApp = nullptr;
     DaemonMenuAction *daemonAction = nullptr;

     QProcess *daemonProcess = nullptr;
     bool dRunning = false;
     bool needRestart = false;
     bool aboutToQuit = false;
     bool foundDaemon = false;

     quint64 lastStateChange = 0;

     //This is for communication between app/daemon
     QSharedMemory sharedMem;
     QTimer *timerDaemon = nullptr;

     //local server for single instance of the app
     QLocalServer *localServer = nullptr;

     //This socket gives us access to the daemon log
     QLocalSocket *logSocket = nullptr;

     bool createSingleApplication();
};

#endif // APPGUI_H
