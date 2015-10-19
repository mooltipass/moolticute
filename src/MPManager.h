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
#ifndef MPMANAGER_H
#define MPMANAGER_H

#include <QtCore>
#include <libusb.h>
#include "MPDevice.h"

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

    void stop();
    MPDevice *getDevice();

signals:
    void mpConnected();
    void mpDisconnected();

private slots:
    void usbDeviceAdded();
    void usbDeviceRemoved();

private:
    MPManager();

    libusb_context *usb_ctx = nullptr;
    MPDevice *currentDev = nullptr;
};

#endif // MPMANAGER_H
