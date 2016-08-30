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

bool g_bEmulationMode = false;

AppDaemon::AppDaemon(int & argc, char ** argv):
    QApplication(argc, argv)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Moolticute Daemon");
    parser.addHelpOption();
    parser.addVersionOption();


    QCommandLineOption emulationMode(QStringList() << "e" << "emulation",
               QCoreApplication::translate("main", "Activate emulation mode, all Websocket API function return emulated string, usefull if you want to try the API."));
    parser.addOption(emulationMode);
    parser.process(QApplication::arguments());
    g_bEmulationMode = parser.isSet(emulationMode);

    //Install and start mp manager instance
    MPManager::Instance();

    wsServer = new WSServer(this);
}

AppDaemon::~AppDaemon()
{
    MPManager::Instance()->stop();
}
