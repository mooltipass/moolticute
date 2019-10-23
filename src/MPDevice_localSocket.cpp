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
    // Fortunately, we speak the "aux" protocol, which has fixed-size packets
    if(!socket)
        return;

    for(;;) {
        int bytesRead = socket->read(incomingPacket, sizeof(incomingPacket) - incomingPacketFill);
        
        if(bytesRead < 0)
            return; // socket closed?

        if(bytesRead == 0)
            break;

        incomingPacketFill += bytesRead;
        if(incomingPacketFill == sizeof(incomingPacket)) {
            if(incomingPacket[0] == 0 && incomingPacket[1] == 0) { // USB data packet
                unsigned int payload_length = ((uint8_t)incomingPacket[2])|(((uint8_t)incomingPacket[3])<<8);
                if(payload_length <= 552) {
                    // not so quick. Now we need to split the data into hid packets
                    qDebug() << "Receive data" << QByteArray(incomingPacket+4, payload_length).toHex();
//                    emit platformDataRead(QByteArray(incomingPacket+4, payload_length));

                    int n_hid_packets = (payload_length+61) / 62;

                    for(int p=0;p < n_hid_packets;p++) {
                        unsigned char hidPacket[64];
                        int bytesRemain = payload_length - p * 62;
                        if(bytesRemain > 62)
                            bytesRemain = 62;

                        hidPacket[0] = bytesRemain;
                        hidPacket[1] = (p<<4) | (n_hid_packets-1);
                        memcpy(hidPacket+2, incomingPacket + 4 + p * 62, bytesRemain);

                        QByteArray ba((const char*)hidPacket, bytesRemain+2);
                        qDebug() << "HID" << ba.toHex();
                        emit platformDataRead(ba);
                    }
                }
            }

            incomingPacketFill = 0;
        }
    }
}

void MPDevice_localSocket::platformWrite(const QByteArray &ba)
{
    // we need to emulate bits of the aux mcu here
    // this means reassembling the small hid packets into larger messages
    if(socket) {
        if(ba.length() < 2) {
            qDebug() << "virtual HID packet too short";
            return;
        }

        uint8_t byte0 = ba[0], byte1 = ba[1];

        if(byte0 == 0xff && byte1 == 0xff) {
            expectFlipBit = false;
            outgoingPacketFill = 0;

        } else {
            if(((byte0 >> 7) & 1) == expectFlipBit) {
                // new message
                outgoingPacketFill = 0;
                expectFlipBit = !expectFlipBit;

            } else if(outgoingPacketFill == 0) {
                qWarning() << "virtual HID invalid flip bit";
                return;

            } else if(expectedByte1 != byte1) {
                qWarning() << "virtual HID byte1 sequence incorrect";
                outgoingPacketFill = 0;
                return;
            }

            if((byte0 & 63)+2 != ba.length() || (byte0 & 63) + outgoingPacketFill > 552) {
                qWarning() << "virtual HID packet payload length incorrect";
                outgoingPacketFill = 0;

            } else {
                if(outgoingPacketFill == 0) {
                    memset(outgoingPacket, 0, sizeof(outgoingPacket));
                    outgoingPacketFill = 4;
                }

                memcpy(outgoingPacket + outgoingPacketFill, ba.constData() + 2, ba.length()-2);
                outgoingPacketFill += ba.length()-2;
                if(((byte1 & 0xf0) >> 4) == (byte1 & 0x0f)) {
                    // final packet, send !
                    outgoingPacket[2] = (outgoingPacketFill-4) & 0xff;
                    outgoingPacket[3] = ((outgoingPacketFill-4) >> 8) & 0xff;

                    socket->write((const char*)outgoingPacket, sizeof(outgoingPacket));
                    
                    qDebug() << "Reassembled message" << QByteArray(outgoingPacket, outgoingPacketFill).toHex();

                    outgoingPacketFill = 0;
                    
                    if(byte0 & 0x40) {
                        // simulate device ack
                        emit platformDataRead(ba);
                    }
                }
                expectedByte1 += 0x10;
            }
        }
    }
}

void MPDevice_localSocket::platformRead()
{
    // nothing to do
}
