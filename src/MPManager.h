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
#ifndef MPMANAGER_H
#define MPMANAGER_H

#include <QtCore>

#if defined(Q_OS_WIN)
#include "MPDevice_win.h"
#elif defined(Q_OS_MAC)
#include "MPDevice_mac.h"
#elif defined(Q_OS_LINUX)
#include "MPDevice_linux.h"
#endif
#include "MPDevice_emul.h"

class MPManager: public QObject
{
    Q_OBJECT
public:
    static MPManager *Instance()
    {
        static MPManager inst;
        return &inst;
    }
    virtual ~MPManager();

    bool initialize();

    void stop();
    MPDevice* getDevice(int at);
    int getDeviceCount() { return devices.count(); }

signals:
    void mpConnected(MPDevice *device);
    void mpDisconnected(MPDevice *device);
    void sendNotification(QString message);

private slots:
    void usbDeviceAdded();
    void usbDeviceRemoved();
    void usbDeviceAdded(QString path);
#if defined(Q_OS_LINUX)
    void usbDeviceAdded(QString path, bool isBLE, bool isBT);
#endif
    void usbDeviceRemoved(QString path);

private:
    MPManager();

    void checkUsbDevices();
    void checkLocalSocketDevice();
    bool isLocalSocketDeviceConnected();
    void disconnectLocalSocketDevice();
    bool isBLEConnectedWithUsb();
    bool isBLEConnectedWithBT();
    void disconnectingDevices();

    QHash<QString, MPDevice *> devices;
};

#endif // MPMANAGER_H
