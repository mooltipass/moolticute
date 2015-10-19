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

}

void MPManager::usbDeviceRemoved()
{

}
