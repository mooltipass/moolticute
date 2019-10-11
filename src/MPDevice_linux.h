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

struct MPPlatformDef
{
    QString id; //unique id for all platform

    bool isBLE = false;
    bool isBluetooth = false;
    QString path;
};

inline bool operator==(const MPPlatformDef &lhs, const MPPlatformDef &rhs) { return lhs.id == rhs.id; }
inline bool operator!=(const MPPlatformDef &lhs, const MPPlatformDef &rhs) { return !(lhs == rhs); }

class MPDevice_linux: public MPDevice
{
    Q_OBJECT
public:
    MPDevice_linux(QObject *parent, const MPPlatformDef &platformDef);
    virtual ~MPDevice_linux();

    //Static function for enumerating devices on platform
    static QList<MPPlatformDef> enumerateDevices();
    static int getDescriptorSize(const char* devpath);
    /**
     * @brief checkDevice
     * Checking if the device is a mooltipass device
     * @param path to the device
     * @param isBLE out param, true if device is a ble
     * @param isBT out param, true if device is connected with BT
     * @return true, if the device is mini/ble
     */
    static bool checkDevice(struct udev_device *raw_dev, bool &isBLE, bool &isBT);
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

    //Bufferize the data sent by sending 64bytes packet at a time
    QQueue<QByteArray> sendBuffer;
    bool failToWriteLogged = false;
};

#endif // MPDEVICE_LINUX_H
