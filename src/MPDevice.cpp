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
#include "MPDevice.h"

MPDevice::MPDevice(QObject *parent, libusb_context *ctx, libusb_device *dev):
    QObject(parent),
    usb_ctx(ctx),
    device(dev)
{
    set_status(Common::UnknownStatus);

    int res = libusb_open(device, &devicefd);
    if (res < 0)
        qWarning() << "Error opening usb device: " << libusb_strerror((enum libusb_error)res);

    usbSendData(MP_MOOLTIPASS_STATUS);
}

MPDevice::~MPDevice()
{
    libusb_close(devicefd);
}

/* Helper class to handle transfer and detach/reattach kernel driver
 */
class USBTransfer: public QObject
{
public:
    USBTransfer(libusb_device_handle *_fd, int intf, MPDevice *parent):
        QObject(parent), fd(_fd), interface(intf), device(parent)
    {
        if (libusb_kernel_driver_active(fd, interface))
        {
            detached_kernel = true;
            libusb_detach_kernel_driver(fd, interface);
        }
        libusb_claim_interface(fd, interface);
        recvData.resize(64);
    }
    ~USBTransfer()
    {
        libusb_release_interface(fd, interface);
        if (detached_kernel)
            libusb_attach_kernel_driver(fd, interface);
    }

    libusb_device_handle *fd;
    int interface = 0;
    bool detached_kernel = false;
    MPDevice *device;
    QByteArray recvData;
};

//Called when a send transfer has completed
void _usbSendCallback(struct libusb_transfer *trf)
{
    qDebug() << "Send callback";

    // /!\ every libusb callbacks are running on the libusb thread!
    USBTransfer *t = reinterpret_cast<USBTransfer *>(trf->user_data);
    QMetaObject::invokeMethod(t->device, "usbSendCb",
                              Qt::QueuedConnection,
                              Q_ARG(struct libusb_transfer *, trf));
}

//Start a send request, _usbSendCallback will be called after completion
void MPDevice::usbSendData(unsigned char cmd, const QByteArray &data)
{
    qDebug() << "Send command: " << QString("0x%1").arg(cmd, 2, 16, QChar('0'));
    QByteArray ba;
    ba.append(data.size());
    ba.append(cmd);
    ba.append(data);

    struct libusb_transfer *trf = libusb_alloc_transfer(1);

    //Our qobject wrapper to maintain data and pass over threads
    USBTransfer *transfer = new USBTransfer(devicefd, 0, this);

    //Send data
    libusb_fill_interrupt_transfer(trf,
                                   devicefd,
                                   LIBUSB_ENDPOINT_OUT | 2,
                                   (unsigned char *)ba.data(),
                                   ba.size(),
                                   _usbSendCallback,
                                   transfer,
                                   1000);

    int err = libusb_submit_transfer(trf);
    if (err)
        qWarning() << "Error sending data: " << libusb_strerror((enum libusb_error)err);
}

void MPDevice::usbSendCb(libusb_transfer *trf)
{
    qDebug() << "Send callback main thread";

    USBTransfer *transfer = reinterpret_cast<USBTransfer *>(trf->user_data);

    if (trf->status != LIBUSB_TRANSFER_COMPLETED)
    {
        qWarning() << "Failed to transfer data to usb endpoint (OUT)";
        libusb_free_transfer(trf);
        transfer->deleteLater();
        return;
    }

    libusb_free_transfer(trf);

    //Ask for a request
    usbRequestReceive(transfer);
}

//Called when a receive transfer has completed
void _usbReceiveCallback(struct libusb_transfer *trf)
{
    // /!\ every libusb callbacks are running on the libusb thread!
    USBTransfer *t = reinterpret_cast<USBTransfer *>(trf->user_data);
    QMetaObject::invokeMethod(t->device, "usbReceiveCb",
                              Qt::QueuedConnection,
                              Q_ARG(struct libusb_transfer *, trf));
}

void MPDevice::usbRequestReceive(USBTransfer *transfer)
{
    struct libusb_transfer *trf = libusb_alloc_transfer(1);

    //Receive data
    libusb_fill_interrupt_transfer(trf,
                                   devicefd,
                                   LIBUSB_ENDPOINT_IN | 1,
                                   (unsigned char *)transfer->recvData.data(),
                                   transfer->recvData.size(),
                                   _usbReceiveCallback,
                                   transfer,
                                   1000);

    int err = libusb_submit_transfer(trf);
    if (err)
        qWarning() << "Error receiving data: " << libusb_strerror((enum libusb_error)err);
}

void MPDevice::usbReceiveCb(libusb_transfer *trf)
{
    USBTransfer *transfer = reinterpret_cast<USBTransfer *>(trf->user_data);
    transfer->deleteLater();

    qDebug() << "Received answer done, " << trf->length << " : " << transfer->recvData;

    if (trf->status != LIBUSB_TRANSFER_COMPLETED)
    {
        qWarning() << "Failed to transfer data to usb endpoint (IN)";
        libusb_free_transfer(trf);
        return;
    }
    libusb_free_transfer(trf);

    switch ((unsigned char)transfer->recvData.at(1))
    {
    case MP_MOOLTIPASS_STATUS:
        qDebug() << "received MP_MOOLTIPASS_STATUS: " << (int)transfer->recvData.at(2);
        set_status((Common::MPStatus)transfer->recvData.at(2));
        break;

    default: break;
    }
}
