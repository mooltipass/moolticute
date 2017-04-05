/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Moolticute is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Moolticute is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
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
#include <QtAwesome.h>

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

    static QtAwesome *qtAwesome();

private slots:
    void connectedChanged();
    void updateSystrayTooltip();
    void searchDaemonTick();
    void slotConnectionEstablished();
    void daemonLogRead();
    void checkUpdate();

private:
     MainWindow *win = nullptr;
     QSystemTrayIcon *systray = nullptr;
     WSClient *wsClient = nullptr;
     QAction *showConfigApp = nullptr;
     DaemonMenuAction *daemonAction = nullptr;

     QProcess *daemonProcess = nullptr;
     QProcess *sshAgentProcess = nullptr;
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
     void startSSHAgent();
};

#endif // APPGUI_H
