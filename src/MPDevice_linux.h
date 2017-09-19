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

#include <libusb.h>
#include "MPDevice.h"

#include <QThread>

class MPPlatformDef
{
public:
    QString id; //unique id for all platform

    libusb_context *ctx = nullptr;
    libusb_device *dev = nullptr;

};

inline bool operator==(const MPPlatformDef &lhs, const MPPlatformDef &rhs) { return lhs.id == rhs.id; }
inline bool operator!=(const MPPlatformDef &lhs, const MPPlatformDef &rhs) { return !(lhs == rhs); }

class USBTransfer;

class TransferWorker : public QObject
{
    Q_OBJECT
public:
    TransferWorker(libusb_context *usb_ctx);
public slots:
    void loop();
    void read(USBTransfer *transfer);
    void write(USBTransfer *transfer, const QByteArray &ba);

private:
    libusb_context *usb_context;
};


class MPDevice_linux: public MPDevice
{
    Q_OBJECT
public:
    MPDevice_linux(QObject *parent, const MPPlatformDef &platformDef);
    virtual ~MPDevice_linux();

    //Static function for enumerating devices on platform
    static QList<MPPlatformDef> enumerateDevices();

public slots:
    virtual void platformRead();
    virtual void platformWrite(const QByteArray &data);

private:
    bool detached_kernel = false;
    libusb_context *usb_ctx;
    libusb_device *device;
    libusb_device_handle *devicefd;

    TransferWorker* worker;
    QThread workerThread;

    void usbSendData(unsigned char cmd, const QByteArray &data = QByteArray());
    void usbRequestReceive();

    friend void _usbSendCallback(struct libusb_transfer *trf);
    friend void _usbReceiveCallback(struct libusb_transfer *trf);
};

#endif // MPDEVICE_LINUX_H
