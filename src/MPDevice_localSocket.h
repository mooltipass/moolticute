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
#ifndef MPDEVICE_LOCALSOCKET_H
#define MPDEVICE_LOCALSOCKET_H

#include "MPDevice.h"
#include <QLocalServer>
#include <QLocalSocket>

struct MPLocalDef {
    QString id;
};

class MPDeviceLocalMonitor: public QObject {
    Q_OBJECT
    friend class MPDevice_localSocket;

public:
    MPDeviceLocalMonitor();
    QList<MPLocalDef> enumerateDevices() const;

signals:
    void localDeviceAdded(QString id);
    void localDeviceRemoved(QString id);

private slots:
    void acceptIncomingConnection();
    void connectionLost();

private:
    QLocalServer server;
    QMap<QString, QLocalSocket *> sockets;

    int nextId = 0;
};

class MPDevice_localSocket: public MPDevice
{
    Q_OBJECT

public:
    MPDevice_localSocket(QObject *parent, const MPLocalDef & def);
    virtual ~MPDevice_localSocket();

    static MPDeviceLocalMonitor *MonitorInstance() {
        static MPDeviceLocalMonitor inst;
        return &inst;
    }

    static QList<MPLocalDef> enumerateDevices() {
        return MonitorInstance()->enumerateDevices();
    }

private slots: 
    void readData();

private:
    virtual void platformRead();
    virtual void platformWrite(const QByteArray &data);

    QPointer<QLocalSocket> socket;
    
    // incoming messages
    uint8_t incomingPacket[64];
    int incomingPacketFill = 0;
};

#endif // MPDEVICE_LOCALSOCKET_H
