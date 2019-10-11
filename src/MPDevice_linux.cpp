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

int MPDevice_linux::INVALID_VALUE = -1;

MPDevice_linux::MPDevice_linux(QObject *parent, const MPPlatformDef &platformDef):
    MPDevice(parent),
    devPath(platformDef.path)
{
    if (platformDef.isBLE)
    {
        deviceType = DeviceType::BLE;
        isBluetooth = platformDef.isBluetooth;
    }
    setupMessageProtocol();

    devfd = open(devPath.toLocal8Bit(), O_RDWR);
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
        sockNotifWrite->setEnabled(true);
    }
}

MPDevice_linux::~MPDevice_linux()
{
    delete sockNotifRead;
    delete sockNotifWrite;

    if (devfd > 0)
    {
        ::close(devfd);
    }
}

void MPDevice_linux::readyRead(int fd)
{
    QByteArray recvData;
    recvData.resize(64);
    ssize_t sz = ::read(fd, recvData.data(), 64);

    if (sz < 0)
    {
        /**
          * If usb is removed it is keep spamming the log
          * with failed message if I do not wrap with this
          * failToWriteLogged bool.
          */
        if (!failToWriteLogged)
        {
            qWarning() << "Failed to read from device: " << strerror(errno);
        }
        failToWriteLogged = true;
    }
    else
    {
        if (isBluetooth)
        {
            emit platformDataRead(recvData.remove(0,1));
        }
        else
        {
            emit platformDataRead(recvData);
        }

        failToWriteLogged = false;
    }

    writeNextPacket();
}

//Start a send request, buffer the data if needed
void MPDevice_linux::platformWrite(const QByteArray &ba)
{
    sendBuffer.enqueue(ba);
    writeNextPacket();
}

int MPDevice_linux::getDescriptorSize(const char *devpath)
{
    int descSize = 0;
    auto fd = open(devpath, O_RDONLY);
    if (fd != INVALID_VALUE)
    {
        int res = ioctl(fd, HIDIOCGRDESCSIZE, &descSize);
        if (res == INVALID_VALUE)
        {
            qDebug() << "Getting descriptor size failed.";
        }
        close(fd);
    }
    return descSize;
}

bool MPDevice_linux::checkDevice(struct udev_device *raw_dev, bool &isBLE, bool &isBT)
{
    int bus_type = 0;
    unsigned short dev_vid = 0;
    unsigned short dev_pid = 0;

    const char *dev_path = udev_device_get_devnode(raw_dev);

    struct udev_device *hid_dev = udev_device_get_parent_with_subsystem_devtype(raw_dev, "hid", nullptr);

    if (!hid_dev)
    {
        udev_device_unref(raw_dev);
        return false;
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
            dev_vid = static_cast<quint16>(idval.at(1).toInt(nullptr, 16));
            dev_pid = static_cast<quint16>(idval.at(2).toInt(nullptr, 16));
        }
    }

    bool isMini = dev_vid == MOOLTIPASS_VENDORID && dev_pid == MOOLTIPASS_PRODUCTID;
    bool isBle = dev_vid == MOOLTIPASS_BLE_VENDORID && dev_pid == MOOLTIPASS_BLE_PRODUCTID;
    if ((bus_type == BUS_USB || bus_type == BUS_BLUETOOTH) &&
        (isMini || isBle))
    {
        const auto descSize = getDescriptorSize(dev_path);
        if (MOOLTIPASS_USBHID_DESC_SIZE == descSize ||
            MOOLTIPASS_BLEHID_DESC_SIZE == descSize)
        {
            isBLE = isBle;
            isBT = bus_type == BUS_BLUETOOTH;
            return true;
        }
    }

    udev_device_unref(raw_dev);
    return false;
}

void MPDevice_linux::writeNextPacket()
{
    if (sendBuffer.isEmpty())
    {
        return; //nothing to write anymore
    }

    QByteArray ba = sendBuffer.dequeue();
    /**
      * Adding a plus 0x00 or 0x03 byte before the message
      * for setting the report number.
      */
    const auto reportId =  static_cast<char>(isBluetooth ? 0x03 : 0x00);
    ba.insert(0, static_cast<char>(reportId));
    ssize_t res = ::write(devfd, ba.data(), static_cast<size_t>(ba.size()));

    if (res < 0)
    {
        qWarning() << "Failed to write data to device: " << strerror(errno);
    }
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
        const char *sysfs_path = udev_list_entry_get_name(dev);
        struct udev_device *raw_dev = udev_device_new_from_syspath(udev, sysfs_path);
        bool isBLE, isBT;
        if (checkDevice(raw_dev, isBLE, isBT))
        {
            const char *dev_path = udev_device_get_devnode(raw_dev);
            MPPlatformDef def;
            def.path = QString::fromUtf8(dev_path);
            def.id = def.path;
            def.isBLE = isBLE;
            def.isBluetooth = isBT;
            devlist << def;

            qDebug() << "Found mooltipass: " << def.path;
        }
    }

    udev_enumerate_unref(enumerate);
    udev_unref(udev);

    return devlist;
}
