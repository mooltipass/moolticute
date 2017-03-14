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
#include "MPDevice_linux.h"
#include "UsbMonitor_linux.h"

MPDevice_linux::MPDevice_linux(QObject *parent, const MPPlatformDef &platformDef):
    MPDevice(parent),
    usb_ctx(platformDef.ctx),
    device(platformDef.dev)
{
    int res = libusb_open(device, &devicefd);
    if (res < 0)
        qWarning() << "Error opening usb device: " << libusb_strerror((enum libusb_error)res);
    else
    {
        if (libusb_kernel_driver_active(devicefd, 0))
        {
            detached_kernel = true;
            libusb_detach_kernel_driver(devicefd, 0);
        }
        libusb_claim_interface(devicefd, 0);

        platformRead();
    }
}

MPDevice_linux::~MPDevice_linux()
{
    libusb_release_interface(devicefd, 0);
    if (detached_kernel)
        libusb_attach_kernel_driver(devicefd, 0);
    libusb_close(devicefd);
}

/* Helper class to handle transfer
 */
class USBTransfer: public QObject
{
public:
    USBTransfer(libusb_device_handle *_fd, int intf, MPDevice *parent):
        QObject(parent), fd(_fd), interface(intf), device(parent)
    {
        recvData.resize(64);
    }
    ~USBTransfer()
    {
    }

    libusb_device_handle *fd;
    int interface = 0;
    MPDevice *device;
    QByteArray recvData;
};

//Called when a send transfer has completed
void _usbSendCallback(struct libusb_transfer *trf)
{
//    qDebug() << "Send callback";

    // /!\ every libusb callbacks are running on the libusb thread!
    USBTransfer *t = reinterpret_cast<USBTransfer *>(trf->user_data);
    QMetaObject::invokeMethod(t->device, "usbSendCb",
                              Qt::QueuedConnection,
                              Q_ARG(struct libusb_transfer *, trf));
}

//Start a send request, _usbSendCallback will be called after completion
void MPDevice_linux::platformWrite(const QByteArray &ba)
{
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
                                   50);

    int err = libusb_submit_transfer(trf);
    if (err)
        qWarning() << "Error sending data: " << libusb_strerror((enum libusb_error)err);
}

void MPDevice_linux::usbSendCb(libusb_transfer *trf)
{
//    qDebug() << "Send callback main thread";

    USBTransfer *transfer = reinterpret_cast<USBTransfer *>(trf->user_data);

    bool error = false;
    if (trf->status != LIBUSB_TRANSFER_COMPLETED)
    {
        qWarning() << "Failed to transfer data to usb endpoint (OUT): " << trf->status << ":" << libusb_strerror((enum libusb_error)trf->status);
        error = true;
    }

    libusb_free_transfer(trf);
    delete transfer;

    if (error)
        emit platformFailed();
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

void MPDevice_linux::platformRead()
{
    //Our qobject wrapper to maintain data and pass over threads
    USBTransfer *transfer = new USBTransfer(devicefd, 0, this);

    struct libusb_transfer *trf = libusb_alloc_transfer(1);

    //Receive data
    libusb_fill_interrupt_transfer(trf,
                                   devicefd,
                                   LIBUSB_ENDPOINT_IN | 1,
                                   (unsigned char *)transfer->recvData.data(),
                                   transfer->recvData.size(),
                                   _usbReceiveCallback,
                                   transfer,
                                   50); //small timeout

    int err = libusb_submit_transfer(trf);
    if (err)
        qWarning() << "Error receiving data: " << libusb_strerror((enum libusb_error)err);
}

void MPDevice_linux::usbReceiveCb(libusb_transfer *trf)
{
    USBTransfer *transfer = reinterpret_cast<USBTransfer *>(trf->user_data);

    if (trf->status == LIBUSB_TRANSFER_COMPLETED)
        emit platformDataRead(transfer->recvData);
    else
        emit platformFailed();

    delete transfer;
    libusb_free_transfer(trf);
    platformRead();
}

QList<MPPlatformDef> MPDevice_linux::enumerateDevices()
{
    QList<MPPlatformDef> devlist;

    // discover devices
    libusb_device **list;

    ssize_t cnt = libusb_get_device_list(UsbMonitor_linux::Instance()->getUsbContext(), &list);
    for (ssize_t i = 0; i < cnt; i++)
    {
        libusb_device *dev = list[i];

        struct libusb_device_descriptor desc;
        struct libusb_config_descriptor *conf_desc = nullptr;
        int res;

        res = libusb_get_device_descriptor(dev, &desc);
        res = libusb_get_active_config_descriptor(dev, &conf_desc);
        if (res < 0)
            libusb_get_config_descriptor(dev, 0, &conf_desc);

        libusb_device_handle *devfd;
        res = libusb_open(dev, &devfd);
        if (res < 0)
        {
            qWarning() << "Error opening usb device: " << libusb_strerror((enum libusb_error)res);
            continue;
        }

        auto getUsbString = [](libusb_device_handle *fd, uint8_t idx)
        {
            char buf[512];
            int len = libusb_get_string_descriptor_ascii(fd, idx, (unsigned char *)buf, sizeof(buf));
            if (len <= 0)
                return QString();
            else
                return QString::fromLocal8Bit(buf, len);
        };

        qDebug() << "Found device vid(" <<
                    QString("0x%1").arg(desc.idVendor, 4, 16, QChar('0')) <<
                    ") pid(" <<
                    QString("0x%1").arg(desc.idProduct, 4, 16, QChar('0')) <<
                    ") Manufacturer(" <<
                    getUsbString(devfd, desc.iManufacturer) <<
                    ") Product(" <<
                    getUsbString(devfd, desc.iProduct) <<
                    ") Serial(" <<
                    getUsbString(devfd, desc.iSerialNumber) <<
                    ")";

        if (conf_desc)
        {
            for (int j = 0;j < conf_desc->bNumInterfaces;j++)
            {
                const struct libusb_interface *intf = &conf_desc->interface[j];
                for (int k = 0;k < intf->num_altsetting;k++)
                {
                    const struct libusb_interface_descriptor *intf_desc = &intf->altsetting[k];
                    if (intf_desc->bInterfaceClass == LIBUSB_CLASS_HID)
                    {
                        if (desc.idVendor == MOOLTIPASS_VENDORID &&
                            desc.idProduct == MOOLTIPASS_PRODUCTID)
                        {
                            MPPlatformDef def;
                            def.ctx = UsbMonitor_linux::Instance()->getUsbContext();
                            def.dev = dev;
                            def.id = QString("%1").arg((quint64)dev); //use dev pointer for ID
                            devlist << def;
                        }
                    }
                }
            }
        }
    }
    libusb_free_device_list(list, 1);

    return devlist;
}
