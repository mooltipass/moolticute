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
#ifndef APPDAEMON_H
#define APPDAEMON_H

#include <QObject>
#include <QSettings>
#include <QLocalServer>

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
#include <QApplication>
#define QAPP QApplication
#else
#include <QCoreApplication>
#define QAPP QCoreApplication
#endif

#include "Common.h"
#include "MPManager.h"
#include "WSServer.h"
#include "HttpServer.h"

class AppDaemon: public QAPP
{
    Q_OBJECT
public:
    AppDaemon(int &argc, char **argv);
    virtual ~AppDaemon();

    bool initialize();

    static bool isEmulationMode();

private:
    WSServer *wsServer;
    HttpServer *httpServer = nullptr;

    //This is for communication between app/daemon
    QSharedMemory sharedMem;

    //this is to send out logs to gui app
    QLocalServer *localLogServer = nullptr;

    static bool emulationMode;
};

#endif // APPDAEMON_H
