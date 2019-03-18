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
#include <QSocketNotifier>
#include <libudev.h>
#include <QTimer>

QString UsbMonitor_linux::ADD_ACTION = "add";
QString UsbMonitor_linux::REMOVE_ACTION = "remove";

UsbMonitor_linux::UsbMonitor_linux()
{
    struct udev* udev = udev_new();
    mon = udev_monitor_new_from_netlink(udev, "udev");

    udev_monitor_filter_add_match_subsystem_devtype(mon, "usb", nullptr);
    udev_monitor_enable_receiving(mon);

    int fd = udev_monitor_get_fd(mon);

    sockMonitor = new QSocketNotifier(fd, QSocketNotifier::Read);
    sockMonitor->setEnabled(true);
    connect(sockMonitor, &QSocketNotifier::activated, this, &UsbMonitor_linux::monitorUSB);
}

UsbMonitor_linux::~UsbMonitor_linux()
{
    delete sockMonitor;
}

void UsbMonitor_linux::monitorUSB(int fd)
{
    Q_UNUSED(fd);
    const auto dev = udev_monitor_receive_device(mon);
    if (dev)
    {
        QString node(udev_device_get_devnode(dev));
        QString action(udev_device_get_action(dev));
        qDebug() << "Node: " << node;
        //qDebug() << "Subsystem: " << udev_device_get_subsystem(dev);
        //qDebug() << "Devtype: " << udev_device_get_devtype(dev);
        qDebug() << "Action: " << action;
        if (ADD_ACTION == action && node.contains("usb"))
        {
            /**
              * Need to add a delay, because without
              * it the device haven't detected yet
              * during usb enumeration.
              */
            QTimer::singleShot(100, [this]()
            {
                emit usbDeviceAdded();
            });
        }
        else if (REMOVE_ACTION == action)
        {
            emit usbDeviceRemoved();
        }
        udev_device_unref(dev);
    }
    else
    {
        printf("No Device from receive_device(). An error occured.\n");
    }
}
