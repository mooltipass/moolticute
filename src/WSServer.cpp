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
#include "WSServer.h"
#include "WSServerCon.h"
#include "AppDaemon.h"

WSServer::WSServer()
{
}

bool WSServer::initialize()
{
    wsServer = new QWebSocketServer(QStringLiteral("Moolticute Server"),
                                    QWebSocketServer::NonSecureMode,
                                    this);

    if (wsServer->listen(AppDaemon::getListenAddress(), MOOLTICUTE_DAEMON_PORT))
    {
        qDebug() << "Moolticute daemon websocket server listening on port " << MOOLTICUTE_DAEMON_PORT;
        connect(wsServer, &QWebSocketServer::newConnection, this, &WSServer::onNewConnection);
    }
    else
    {
        qCritical() << "Failed to listen on port " << MOOLTICUTE_DAEMON_PORT;
        return false;
    }

    connect(MPManager::Instance(), SIGNAL(mpConnected(MPDevice*)), this, SLOT(mpAdded(MPDevice*)));
    connect(MPManager::Instance(), SIGNAL(mpDisconnected(MPDevice*)), this, SLOT(mpRemoved(MPDevice*)));

    if (MPManager::Instance()->getDeviceCount() > 0)
        mpAdded(MPManager::Instance()->getDevice(0));

    return true;
}

WSServer::~WSServer()
{
    wsServer->close();
    qDeleteAll(wsClients.begin(), wsClients.end());
}

void WSServer::onNewConnection()
{
    QWebSocket *wsocket = wsServer->nextPendingConnection();

    connect(wsocket, &QWebSocket::disconnected, this, &WSServer::socketDisconnected);
    WSServerCon *c = new WSServerCon(wsocket);
    c->resetDevice(device);
    c->sendInitialStatus();
    //let clients send broadcast messages
    connect(c, &WSServerCon::notifyAllClients, this, &WSServer::notifyClients);
    connect(c, &WSServerCon::sendMessageToGUI, this, &WSServer::notifyGUI);

    wsClients[wsocket] = c;
    wsClientsReverse[c] = wsocket;

    qDebug() << "New connection";
}

void WSServer::socketDisconnected()
{
    QWebSocket *wsocket = qobject_cast<QWebSocket *>(sender());
    if (wsocket && wsClients.contains(wsocket))
    {
        qDebug() << "Connection closed " << wsClients[wsocket];

        if (isMemModeLocked() &&
            lockedUid == wsClients[wsocket]->getClientUid())
        {
            qWarning() << "Exiting MMM because client exits without doing it.";
            device->exitMemMgmtMode();
        }

        wsClients[wsocket]->deleteLater();
        wsClientsReverse.remove(wsClients[wsocket]);
        wsClients.remove(wsocket);
    }
}

void WSServer::notifyClients(const QJsonObject &obj)
{
    for (auto it = wsClients.begin();it != wsClients.end();it++)
    {
        it.value()->sendJsonMessage(obj);
    }
}

void WSServer::notifyGUI(const QString &message, bool &isGuiRunning)
{
    for (auto it = wsClients.begin(); it != wsClients.end(); ++it)
    {
        //Notify GUI
        if (it.key()->resourceName().contains("localhost") && !(it.key()->origin().contains("extension")))
        {
            it.value()->sendJsonMessage(message);
            isGuiRunning = true;
            return;
        }
    }
    isGuiRunning = false;
}

void WSServer::mpAdded(MPDevice *dev)
{
    if (device == dev)
        return;

    //tell clients that current mp is not connected anymore
    //and use new connected mp instead
    if (device && device != dev)
        mpRemoved(device);

    qDebug() << "Mooltipass connected";
    device = dev;

    for (auto it = wsClients.begin();it != wsClients.end();it++)
    {
        it.value()->resetDevice(device);
    }
}

void WSServer::mpRemoved(MPDevice *dev)
{
    if (device == dev)
    {
        qDebug() << "Mooltipass disconnected";
        device = nullptr;

        for (auto it = wsClients.begin();it != wsClients.end();it++)
        {
            it.value()->resetDevice(device);
        }
    }
}

bool WSServer::checkClientExists(WSServerCon *wscon)
{
    if (!wsClientsReverse.contains(wscon))
        return false;
    return true;
}

bool WSServer::checkClientExists(QWebSocket *ws)
{
    if (!wsClients.contains(ws))
        return false;
    return true;
}

bool WSServer::isMemModeLocked(QString uid)
{
    //if the current client that has locked
    //the mem mode query for locked state, return false
    if (uid == lockedUid)
        return false;

    //if mem mode is enabled, it is locked
    if (device && device->get_memMgmtMode())
        return true;
    return false;
}
