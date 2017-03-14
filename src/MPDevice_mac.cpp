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
#include "MPDevice_mac.h"
#include "UsbMonitor_mac.h"
#include <QtConcurrent/QtConcurrent>

void _read_report_callback(void *context,
                           IOReturn result,
                           void *sender,
                           IOHIDReportType report_type,
                           uint32_t report_id,
                           uint8_t *report,
                           CFIndex report_length)
{
    Q_UNUSED(result);
    Q_UNUSED(sender);
    Q_UNUSED(report_type);
    Q_UNUSED(report_id);

    MPDevice_mac *dev = reinterpret_cast<MPDevice_mac *>(context);
    QByteArray data((const char *)report, report_length);
    emit dev->platformDataRead(data);
}

MPDevice_mac::MPDevice_mac(QObject *parent, const MPPlatformDef &platformDef):
    MPDevice(parent),
    hidref(platformDef.hidref)
{
    IOReturn ret = IOHIDDeviceOpen(hidref, kIOHIDOptionsTypeSeizeDevice);
    if (ret != kIOReturnSuccess)
    {
        qWarning() << "IOHIDDeviceOpen: Error opening device";
        return;
    }
    CFRetain(hidref);

    //Get max input report length
    CFTypeRef ref = IOHIDDeviceGetProperty(hidref, CFSTR(kIOHIDMaxInputReportSizeKey));
    if (ref && CFGetTypeID(ref) == CFNumberGetTypeID())
        CFNumberGetValue((CFNumberRef)ref, kCFNumberSInt32Type, &maxInReportLen);

    readBuf.resize(maxInReportLen);

    //Attach a callback that will be triggered when data comes in
    IOHIDDeviceRegisterInputReportCallback(hidref,
                                           (uint8_t *)readBuf.data(),
                                           readBuf.size(),
                                           &_read_report_callback,
                                           this);
}

MPDevice_mac::~MPDevice_mac()
{
    CFRelease(hidref);
}

QList<MPPlatformDef> MPDevice_mac::enumerateDevices()
{
    return UsbMonitor_mac::Instance()->getDeviceList();
}

void MPDevice_mac::platformWrite(const QByteArray &data)
{
    //Do the write operation in a thread to avoid blocking
    QtConcurrent::run([=]()
    {
        IOReturn res = IOHIDDeviceSetReport(hidref,
                                            kIOHIDReportTypeOutput,
                                            0,
                                            (const uint8_t *)data.constData(),
                                            data.size());
        if (res != kIOReturnSuccess)
            qWarning() << "Failed to write data to device";
    });
}

void MPDevice_mac::platformRead()
{
    //Nothing, everything is done by callback from IOHIDDevice in _read_report_callback
}
