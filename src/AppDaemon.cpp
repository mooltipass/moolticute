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
#include "AppDaemon.h"
#include "HttpServer.h"
#include "Common.h"
#include "version.h"

#ifdef Q_OS_MAC
#include "MacUtils.h"
#endif

bool AppDaemon::emulationMode = false;

AppDaemon::AppDaemon(int &argc, char **argv):
    QAPP(argc, argv),
    sharedMem("moolticute")
{
}

bool AppDaemon::initialize()
{
    localLogServer = new QLocalServer(this);
    localLogServer->removeServer(MOOLTICUTE_DAEMON_LOG_SOCK);
    localLogServer->listen(MOOLTICUTE_DAEMON_LOG_SOCK);
    Common::installMessageOutputHandler(localLogServer);

    QCoreApplication::setOrganizationName("Raoulh");
    QCoreApplication::setOrganizationDomain("raoulh.org");
    QCoreApplication::setApplicationName("Moolticute");

    qInfo() << "Moolticute Daemon version: " << APP_VERSION;
    qInfo() << "(c) 2016 Raoul Hecky";
    qInfo() << "https://github.com/raoulh/moolticute";
    qInfo() << "------------------------------------";

#ifdef Q_OS_MAC
    utils::mac::hideDockIcon(true);
#endif

    //Create the shared memory segment. The segment will be used by the AppGui to find the daemon
    //It also prevents the daemon to be started multiple time. Only one instance allowed.
    if (!sharedMem.create(SHMEM_SIZE, QSharedMemory::ReadWrite))
    {
        //The shared mem exists, read the content and check if the process with the PID exists
        if (!sharedMem.attach())
        {
            qCritical() << "Failed to attach to shared segment: " << sharedMem.errorString();
            return false;
        }

        QJsonObject obj = Common::readSharedMemory(sharedMem);

        //PID is stored as string to prevent double conversion in json
        qint64 pid = obj["daemon_pid"].toString().toLongLong();

        if (Common::isProcessRunning(pid))
        {
            qCritical() << "Process is already running.";
            return false;
        }
    }

    //Setup shared memory data
    QJsonObject obj = {{ "daemon_pid", QString::number(qApp->applicationPid()) }};
    if (!Common::writeSharedMemory(sharedMem, obj))
    {
        qCritical() << "Error writing to shared memory";
        return false;
    }

    QCommandLineParser parser;

    parser.setApplicationDescription("Moolticute Daemon");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption emulMode(QStringList() << "e" << "emulation",
                                     QCoreApplication::translate("main", "Activate emulation mode, all Websocket API function return emulated string, usefull if you want to try the API."));
    parser.addOption(emulMode);

    // An option with a value
    QCommandLineOption debugHttpServer(QStringList() << "s" << "debug-http-server",
                                       QCoreApplication::translate("main", "Activate Http Server for debug mode. This mode is used to serve a web page on http://localhost:XXXX/ in order to test/debug the webscoket API easyly."),
                                       QCoreApplication::translate("main", "port"));
    parser.addOption(debugHttpServer);

    parser.process(qApp->arguments());

    emulationMode = parser.isSet(emulMode);

    if (parser.isSet(debugHttpServer))
    {
        httpServer = new HttpServer(this);
        if (!httpServer->start(parser.value(debugHttpServer).toInt()))
        {
            qCritical() << "Fatal error: Failed to create HTTP server.";
            return false;
        }
    }

    //Install and start mp manager instance and ws server
    if (!WSServer::Instance()->initialize())
    {
        qCritical() << "WSServer Fatal error";
        return false;
    }

    QTimer::singleShot(500, []()
    {
        if (!MPManager::Instance()->initialize())
            qCritical() << "USBManager Fatal error";
    });

    return true;
}

AppDaemon::~AppDaemon()
{
    MPManager::Instance()->stop();
    delete httpServer;
}

bool AppDaemon::isEmulationMode()
{
    return emulationMode;
}
