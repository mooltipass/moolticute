/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Calaos is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Calaos is distributed in the hope that it will be useful,
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

bool g_bEmulationMode = false;

AppDaemon::AppDaemon(int &argc, char **argv):
    QApplication(argc, argv)
{
}

bool AppDaemon::initialize()
{
    Common::installMessageOutputHandler();

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

    QCommandLineParser parser;

    parser.setApplicationDescription("Moolticute Daemon");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption emulationMode(QStringList() << "e" << "emulation",
               QCoreApplication::translate("main", "Activate emulation mode, all Websocket API function return emulated string, usefull if you want to try the API."));
    parser.addOption(emulationMode);

    // An option with a value
    QCommandLineOption debugHttpServer(QStringList() << "s" << "debug-http-server",
            QCoreApplication::translate("main", "Activate Http Server for debug mode. This mode is used to serve a web page on http://localhost:4789/debug in order to test/debug the webscoket API easyly."),
            QCoreApplication::translate("main", "port"));
    parser.addOption(debugHttpServer);

    parser.process(QApplication::arguments());

    g_bEmulationMode = parser.isSet(emulationMode);

    if (parser.isSet(debugHttpServer))
    {
        httpServer = new HttpServer(this);
        if (!httpServer->start(parser.value(debugHttpServer).toInt()))
        {
            qCritical() << "Fatal error: Failed to create HTTP server.";
            return false;
        }
    }

    //Install and start mp manager instance
    MPManager::Instance();

    try
    {
        wsServer = new WSServer(this);
    }
    catch (const std::exception &e)
    {
        qCritical() << "Fatal error: " << e.what();
        return false;
    }

    return true;
}

AppDaemon::~AppDaemon()
{
    MPManager::Instance()->stop();
    delete httpServer;
}
