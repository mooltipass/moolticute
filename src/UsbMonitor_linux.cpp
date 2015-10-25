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
#include "UsbMonitor_linux.h"
#include <QtConcurrent/QtConcurrent>

int libusb_device_add_cb(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data)
{
    Q_UNUSED(event);
    Q_UNUSED(ctx);
    UsbMonitor_linux *um = reinterpret_cast<UsbMonitor_linux *>(user_data);
    struct libusb_device_descriptor desc;
    int rc;

    rc = libusb_get_device_descriptor(dev, &desc);
    if (rc != LIBUSB_SUCCESS)
        qWarning() << "Error getting device descriptor";
    else
    {
        emit um->usbDeviceAdded();
        qDebug() << "Device added: " << desc.idVendor << "-" << desc.idProduct;
    }

    return 0;
}

int libusb_device_del_cb(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data)
{
    Q_UNUSED(event);
    Q_UNUSED(ctx);
    UsbMonitor_linux *um = reinterpret_cast<UsbMonitor_linux *>(user_data);
    struct libusb_device_descriptor desc;
    int rc;

    rc = libusb_get_device_descriptor(dev, &desc);
    if (rc != LIBUSB_SUCCESS)
        qWarning() << "Error getting device descriptor";
    else
    {
        qDebug() << "Device removed: " << desc.idVendor << "-" << desc.idProduct;
        emit um->usbDeviceRemoved();
    }

    return 0;
}

UsbMonitor_linux::UsbMonitor_linux()
{
    qRegisterMetaType<struct libusb_transfer *>();

    int err = libusb_init(&usb_ctx);
    if (err < 0 || !usb_ctx)
        qWarning() << "Failed to initialise libusb: " << libusb_error_name(err);

    if (!libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG))
        qDebug() << "libusb Hotplug capabilites are not supported on this platform";
}

void UsbMonitor_linux::start()
{
    int err;
    err = libusb_hotplug_register_callback(usb_ctx,
                                           LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
                                           (libusb_hotplug_flag)0,
                                           vendorId,
                                           productId,
                                           LIBUSB_HOTPLUG_MATCH_ANY,
                                           libusb_device_add_cb,
                                           this,
                                           &cbaddhandle);
    if (err != LIBUSB_SUCCESS)
        qWarning() << "Failed to register LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED callback";

    err = libusb_hotplug_register_callback(usb_ctx,
                                           LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
                                           (libusb_hotplug_flag)0,
                                           vendorId,
                                           productId,
                                           LIBUSB_HOTPLUG_MATCH_ANY,
                                           libusb_device_del_cb,
                                           this,
                                           &cbdelhandle);
    if (err != LIBUSB_SUCCESS)
        qWarning() << "Failed to register LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT callback";

    //Using poll fd does not work on windows,
    //so running in a thread is the only viable solution
    //This also means all callbacks from libusb will be called from the thread.
    //Need to be carefull with that.
    QtConcurrent::run([=]()
    {
        bool r;
        do
        {
            libusb_handle_events(nullptr);
            QMutexLocker l(&mutex);
            r = run;
        } while (r);
    });
}

void UsbMonitor_linux::stop()
{
    if (!run) return;
    {
        QMutexLocker l(&mutex);
        run = false;
    }
    libusb_hotplug_deregister_callback(usb_ctx, cbaddhandle);
    libusb_hotplug_deregister_callback(usb_ctx, cbdelhandle);
}

UsbMonitor_linux::~UsbMonitor_linux()
{
    stop();
    libusb_exit(usb_ctx);
}
