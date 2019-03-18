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
#ifndef MPDEVICE_LINUX_H
#define MPDEVICE_LINUX_H

#include "MPDevice.h"
#include <QSocketNotifier>

#include <QThread>

class MPPlatformDef
{
public:
    QString id; //unique id for all platform

    bool isBLE = false;
    QString path;
};

inline bool operator==(const MPPlatformDef &lhs, const MPPlatformDef &rhs) { return lhs.id == rhs.id; }
inline bool operator!=(const MPPlatformDef &lhs, const MPPlatformDef &rhs) { return !(lhs == rhs); }

class MPDevice_linux: public MPDevice
{
    enum class ExclusiveAccess
    {
        RELEASE = 0,
        GRAB = 1
    };

    Q_OBJECT
public:
    MPDevice_linux(QObject *parent, const MPPlatformDef &platformDef);
    virtual ~MPDevice_linux();

    //Static function for enumerating devices on platform
    static QList<MPPlatformDef> enumerateDevices();
    static int getDescriptorSize(const char* devpath);
    static int INVALID_VALUE;

private slots:
    void readyRead(int fd);
    void writeNextPacket();

private:
    virtual void platformRead();
    virtual void platformWrite(const QByteArray &data);

    QString devPath;
    int devfd = 0; //device fd
    QSocketNotifier *sockNotifRead = nullptr;
    QSocketNotifier *sockNotifWrite = nullptr;

    int grabbed = INVALID_VALUE;

    //Bufferize the data sent by sending 64bytes packet at a time
    QQueue<QByteArray> sendBuffer;
    bool failToWriteLogged = false;
};

#endif // MPDEVICE_LINUX_H
