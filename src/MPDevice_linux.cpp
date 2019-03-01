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

#include <linux/hidraw.h>
#include <linux/version.h>
#include <linux/input.h>
#include <libudev.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

MPDevice_linux::MPDevice_linux(QObject *parent, const MPPlatformDef &platformDef):
    MPDevice(parent),
    devPath(platformDef.path)
{
    devfd = ::open(devPath.toLocal8Bit(), O_RDWR | O_NONBLOCK);
    if (devfd < 0)
    {
        qWarning() << "Error opening usb device: " << strerror(errno);
    }
    else
    {
        sockNotifRead = new QSocketNotifier(devfd, QSocketNotifier::Read);
        sockNotifRead->setEnabled(true);
        connect(sockNotifRead, &QSocketNotifier::activated, this, &MPDevice_linux::readyRead);

        sockNotifWrite = new QSocketNotifier(devfd, QSocketNotifier::Write);
        sockNotifWrite->setEnabled(false);
        connect(sockNotifWrite, &QSocketNotifier::activated, this, &MPDevice_linux::readyWritten);
    }
}

MPDevice_linux::~MPDevice_linux()
{
    delete sockNotifRead;
    delete sockNotifWrite;
    if (devfd > 0)
        ::close(devfd);
}

void MPDevice_linux::readyRead(int fd)
{
    QByteArray recvData;
    recvData.resize(64);
    ssize_t sz = ::read(fd, recvData.data(), 64);

    if (sz < 0)
        qWarning() << "Failed to read from device: " << strerror(errno);

    if (sz > 0) //only emit when there is data
        emit platformDataRead(recvData);

    if (sz > 64)
        qDebug() << "More than 64 bytes...?";
}

//Start a send request, buffer the data if needed
void MPDevice_linux::platformWrite(const QByteArray &ba)
{
    if (ba.size() > 64)
        sendBuffer.enqueue(ba.mid(0, 64));
    else
        sendBuffer.enqueue(ba);

    if (!sockNotifWrite->isEnabled())
        writeNextPacket();
}

void MPDevice_linux::writeNextPacket()
{
    if (sendBuffer.isEmpty())
        return; //nothing to write anymore

    QByteArray ba = sendBuffer.dequeue();

    ssize_t sz = ::write(devfd, ba.data(), ba.size());
    sockNotifWrite->setEnabled(true);

    if (sz < 0)
    {
        qWarning() << "Failed to write data to device: " << strerror(errno);
        QTimer::singleShot(0, this, &MPDevice_linux::writeNextPacket);
    }
}

void MPDevice_linux::readyWritten(int)
{
    sockNotifWrite->setEnabled(false);
    writeNextPacket();
}

void MPDevice_linux::platformRead()
{
    //Nothing to do here
}

QList<MPPlatformDef> MPDevice_linux::enumerateDevices()
{
    QList<MPPlatformDef> devlist;

    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev;

    udev = udev_new();
    if (!udev)
    {
        qWarning() << "Can't create udev object";
        return devlist;
    }

    //List all hidraw devices
    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "hidraw");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(dev, devices)
    {
        int bus_type = 0;
        unsigned short dev_vid = 0;
        unsigned short dev_pid = 0;

        const char *sysfs_path = udev_list_entry_get_name(dev);
        struct udev_device *raw_dev = udev_device_new_from_syspath(udev, sysfs_path);
        const char *dev_path = udev_device_get_devnode(raw_dev);

        struct udev_device *hid_dev = udev_device_get_parent_with_subsystem_devtype(raw_dev, "hid", NULL);

        if (!hid_dev)
        {
            udev_device_unref(raw_dev);
            continue;
        }

        QString uevent = QString::fromUtf8(udev_device_get_sysattr_value(hid_dev, "uevent"));
        for (QString keyval: uevent.split('\n'))
        {
            QStringList kv = keyval.split('=');
            if (kv.size() < 2)
                continue;
            if (kv.at(0) == "HID_ID")
            {
                QStringList idval = kv.at(1).split(':');
                if (idval.size() < 3)
                    continue;
                bus_type = idval.at(0).toInt(nullptr, 16);
                dev_vid = idval.at(1).toInt(nullptr, 16);
                dev_pid = idval.at(2).toInt(nullptr, 16);
            }
        }

        if (bus_type == BUS_USB &&
            dev_vid == MOOLTIPASS_VENDORID &&
            dev_pid == MOOLTIPASS_PRODUCTID)
        {
            MPPlatformDef def;
            def.path = QString::fromUtf8(dev_path);
            def.id = def.path;
            devlist << def;

            qDebug() << "Found mooltipass: " << def.path;
        }

        udev_device_unref(raw_dev);
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    return devlist;
}
