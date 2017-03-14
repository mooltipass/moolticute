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
#include <QSettings>
#include <QCoreApplication>
#include <QStandardPaths>
#include "HttpServer.h"


HttpServer::HttpServer(QObject *parent) :
    QObject(parent),
    m_tcpServer(0)
{

}

HttpServer::~HttpServer()
{

}

bool HttpServer::start(quint16 port)
{
    bool ret = false;
    m_tcpServer = new QTcpServer(this);

    ret = m_tcpServer->listen(QHostAddress(QHostAddress::AnyIPv4), port);
    if (!ret)
    {
        delete m_tcpServer;
        return ret;
    }

    qDebug() << "Http server listening on port " << port;
    connect(m_tcpServer, &QTcpServer::newConnection, [=]()
    {
        while (m_tcpServer->hasPendingConnections())
        {
            QTcpSocket *socket = m_tcpServer->nextPendingConnection();
            HttpClient *client = new HttpClient(socket, m_tcpServer);
            m_clients << client;
        }
    });

    return ret;

}

