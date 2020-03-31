/******************************************************************************
 **  Copyright (c) Andrzej Szombierski. All Rights Reserved.
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
 **  along with Moolticute; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#include "MPDevice_localSocket.h"
#include <QDebug>

MPDeviceLocalMonitor::MPDeviceLocalMonitor()
{
    const char *socketName = "moolticuted_local_dev";
    QLocalServer::removeServer(socketName);
    server.listen(socketName);
    connect(&server, SIGNAL(newConnection()), this, SLOT(acceptIncomingConnection()));
}

void MPDeviceLocalMonitor::acceptIncomingConnection()
{
    while(server.hasPendingConnections()) {
        auto newSocket = server.nextPendingConnection();
        if(!newSocket)
            continue;

        QString id = QString::number(++nextId);
        connect(newSocket, SIGNAL(disconnected()), this, SLOT(connectionLost()));
        sockets[id] = newSocket;
        emit localDeviceAdded(id);
    }
}

void MPDeviceLocalMonitor::connectionLost()
{
    auto socket = static_cast<QLocalSocket*>(sender());

    // this is not the greatest way to search for the socket, but realistically this map will only have one entry
    for(auto it = sockets.begin(); it != sockets.end(); ++it) {
        if(it.value() == socket) {
            QString id = it.key();
            sockets.erase(it);
            emit localDeviceRemoved(id);
            break;
        }
    }

    socket->deleteLater();
}

QList<MPLocalDef> MPDeviceLocalMonitor::enumerateDevices() const
{
    QList<MPLocalDef> ret;
    for(auto it = sockets.begin(); it != sockets.end(); ++it)
        ret.append(MPLocalDef { it.key() });
    return ret;
}


MPDevice_localSocket::MPDevice_localSocket(QObject *parent, const MPLocalDef & def): MPDevice(parent)
{
    socket = MonitorInstance()->sockets[def.id];
    deviceType = DeviceType::BLE;
    isBluetooth = false;

    if(socket)
        connect(socket, SIGNAL(readyRead()), this, SLOT(readData()));

    setupMessageProtocol();
    sendInitMessages();
}

MPDevice_localSocket::~MPDevice_localSocket()
{
}

void MPDevice_localSocket::readData()
{
    // QLocalSocket is stream-based, meaning that it can arbitrarily merge/split data packets
    // We will split the HID packets based on the payload length field
    if(!socket)
        return;

    for(;;) {
        if(incomingPacketFill >= 2) {
            int payloadLength = incomingPacket[0] & 63;
            if(payloadLength > 62) {
                qWarning() << "Invalid HID packet received";
                incomingPacketFill = 0;
                continue;
            }

            if(incomingPacketFill >= 2 + payloadLength) {
                // return one packet
                emit platformDataRead(QByteArray((const char*)incomingPacket, 2 + payloadLength));

                // shift bytes in buffer
                incomingPacketFill -= 2 + payloadLength;
                memmove(incomingPacket, incomingPacket + 2 + payloadLength, incomingPacketFill);
                continue;
            }
        }

        int bytesRead = socket->read((char*)incomingPacket + incomingPacketFill, sizeof(incomingPacket) - incomingPacketFill);

        if(bytesRead < 0) {
            incomingPacketFill = 0;
            return; // socket closed?
        }

        if(bytesRead == 0)
            break;

        incomingPacketFill += bytesRead;
    }
}

void MPDevice_localSocket::platformWrite(const QByteArray &ba)
{
    if(socket)
        socket->write(ba);
}

void MPDevice_localSocket::platformRead()
{
    // nothing to do
}
