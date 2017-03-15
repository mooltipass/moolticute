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
#ifndef WSSERVER_H
#define WSSERVER_H

#include <QtCore>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QJsonDocument>
#include "Common.h"
#include "MPManager.h"
#include "WSServerCon.h"

class WSServer: public QObject
{
    Q_OBJECT
public:
    static WSServer *Instance()
    {
        static WSServer s;
        return &s;
    }
    bool initialize();

    ~WSServer();

    //check if a ws client still exists and has not been disconnected
    bool checkClientExists(WSServerCon *wscon);
    bool checkClientExists(QWebSocket *ws);

private slots:
    void onNewConnection();
    void socketDisconnected();
    void notifyClients(const QJsonObject &obj);

    void mpAdded(MPDevice *device);
    void mpRemoved(MPDevice *device);

private:
    WSServer();
    QWebSocketServer *wsServer = nullptr;
    QHash<QWebSocket *, WSServerCon *> wsClients;
    QHash<WSServerCon *, QWebSocket *> wsClientsReverse; //reverse map for fast lookup

    //Current MP
    //For now only one MP is supported. maybe add multi support
    // one day (but it's not really useful anyway)
    MPDevice *device = nullptr;
};

#endif // WSSERVER_H
