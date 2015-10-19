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
#include "MPManager.h"

#ifdef Q_OS_WIN
#include "UsbMonitor_win.h"
#else
#include "UsbMonitor_linux.h"
#endif

MPManager::MPManager():
    QObject(nullptr)
{
    int err;

    err = libusb_init(&usb_ctx);
    if (err < 0 || !usb_ctx)
        qWarning() << "Failed to initialise libusb: " << libusb_error_name(err);

    if (!libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG))
        qDebug() << "libusb Hotplug capabilites are not supported on this platform";

#ifdef Q_OS_WIN
    connect(UsbMonitor_win::Instance(), SIGNAL(usbDeviceAdded()), this, SLOT(usbDeviceAdded()));
    connect(UsbMonitor_win::Instance(), SIGNAL(usbDeviceRemoved()), this, SLOT(usbDeviceRemoved()));
#else
    UsbMonitor_linux::Instance()->filterVendorId(MOOLTIPASS_VENDORID);
    UsbMonitor_linux::Instance()->filterProductId(MOOLTIPASS_PRODUCTID);
    UsbMonitor_linux::Instance()->start();
    connect(UsbMonitor_linux::Instance(), SIGNAL(usbDeviceAdded()), this, SLOT(usbDeviceAdded()));
    connect(UsbMonitor_linux::Instance(), SIGNAL(usbDeviceRemoved()), this, SLOT(usbDeviceRemoved()));
#endif

    checkUsbDevices();
}

MPManager::~MPManager()
{
    stop();
    libusb_exit(usb_ctx);
}

void MPManager::stop()
{
#ifndef Q_OS_WIN
    UsbMonitor_linux::Instance()->stop();
#endif
}

void MPManager::usbDeviceAdded()
{
    checkUsbDevices();
}

void MPManager::usbDeviceRemoved()
{
    checkUsbDevices();
}

MPDevice *MPManager::getDevice(int at)
{
    if (at < 0 || at >= devices.count())
        return nullptr;
    auto it = devices.begin();
    while (it != devices.end() && at > 0) at--;
    return it.value();
}

QList<MPDevice *> MPManager::getDevices()
{
    QList<MPDevice *> devs;
    auto it = devices.begin();
    while (it != devices.end())
        devs.append(it.value());
    return devs;
}

void MPManager::checkUsbDevices()
{
    // discover devices
    libusb_device **list;
    QList<libusb_device *> detectedDevs;

    qDebug() << "List usb devices...";

    ssize_t cnt = libusb_get_device_list(usb_ctx, &list);
    if (cnt < 0)
    {
        //No USB devices found, means all MPs are gone disconnected
        auto it = devices.begin();
        while (it != devices.end())
        {
            emit mpDisconnected(it.value());
            it.value()->deleteLater();
        }
        devices.clear();
        return;
    }

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
            if (len < 0)
                return QString();
            else
                return QString(buf);
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
                            //This is a new connected mooltipass
                            if (!devices.contains(dev))
                            {
                                devices[dev] = new MPDevice(this, usb_ctx, dev);
                                emit mpConnected(devices[dev]);
                            }

                            detectedDevs.append(dev);
                        }
                    }
                }
            }
        }
    }
    libusb_free_device_list(list, 1);

    //Clear disconnected devices
    auto it = devices.begin();
    while (it != devices.end())
    {
        if (!detectedDevs.contains(it.key()))
        {
            emit mpDisconnected(it.value());
            it.value()->deleteLater();
            devices.remove(it.key());
            it = devices.begin();
        }
    }
    return;
}
