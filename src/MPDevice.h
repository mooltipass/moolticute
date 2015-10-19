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
#ifndef MPDEVICE_H
#define MPDEVICE_H

#include <QObject>
#include "Common.h"
#include <libusb.h>
#include "MooltipassCmds.h"
#include "QtHelper.h"

class USBTransfer;

class MPDevice: public QObject
{
    Q_OBJECT
    QT_WRITABLE_PROPERTY(Common::MPStatus, status)

public:
    MPDevice(QObject *parent, libusb_context *ctx, libusb_device *dev);
    virtual ~MPDevice();

private slots:
    void usbSendCb(struct libusb_transfer *trf);
    void usbReceiveCb(struct libusb_transfer *trf);

private:
    libusb_context *usb_ctx;
    libusb_device *device;
    libusb_device_handle *devicefd;

    void usbSendData(unsigned char cmd, const QByteArray &data = QByteArray());
    void usbRequestReceive(USBTransfer *transfer);

    friend void _usbSendCallback(struct libusb_transfer *trf);
    friend void _usbReceiveCallback(struct libusb_transfer *trf);
};

Q_DECLARE_METATYPE(struct libusb_transfer *)

#endif // MPDEVICE_H
