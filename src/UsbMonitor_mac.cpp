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
#include "UsbMonitor_mac.h"

void _device_matching_callback(void *user_data,
                               IOReturn inResult, // the result of the matching operation
                               void *inSender, // the IOHIDManagerRef for the new device
                               IOHIDDeviceRef inIOHIDDeviceRef) // the new HID device
{
    Q_UNUSED(inResult);
    Q_UNUSED(inSender);
    UsbMonitor_mac *um = reinterpret_cast<UsbMonitor_mac *>(user_data);

    MPPlatformDef def;
    def.hidref = inIOHIDDeviceRef;
    def.id = QString("%1").arg((quint64)def.hidref);

    if (!um->deviceHash.contains(def.hidref))
    {
        um->deviceHash[def.hidref] = def;
        emit um->usbDeviceAdded();
        qDebug() << "Device added: " << def.id;
    }
}

void _device_removal_callback(void *user_data,
                              IOReturn inResult, // the result of the removing operation
                              void *inSender, // the IOHIDManagerRef for the device being removed
                              IOHIDDeviceRef inIOHIDDeviceRef) // the removed HID device
{
    Q_UNUSED(inResult);
    Q_UNUSED(inSender);
    UsbMonitor_mac *um = reinterpret_cast<UsbMonitor_mac *>(user_data);

    if (um->deviceHash.contains(inIOHIDDeviceRef))
    {
        MPPlatformDef def = um->deviceHash[inIOHIDDeviceRef];
        um->deviceHash.remove(inIOHIDDeviceRef);
        emit um->usbDeviceRemoved();
        qDebug() << "Device removed: " << def.id;
    }
}

UsbMonitor_mac::UsbMonitor_mac():
    QObject()
{
    // Create an HID Manager
    hidmanager = IOHIDManagerCreate(kCFAllocatorDefault,
                                                    kIOHIDOptionsTypeNone);

    // Create a Matching Dictionary for filtering HID devices
    CFMutableDictionaryRef matchDict = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                                 4,
                                                                 &kCFTypeDictionaryKeyCallBacks,
                                                                 &kCFTypeDictionaryValueCallBacks);

    // Set device info in the Matching Dictionary (pid/vid/usage/usagepage)

    //VendorID
    int val = MOOLTIPASS_VENDORID;
    CFNumberRef vid = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &val);
    CFDictionarySetValue(matchDict, CFSTR(kIOHIDVendorIDKey), vid);
    CFRelease(vid);

    //ProductID
    val = MOOLTIPASS_PRODUCTID;
    CFNumberRef pid = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &val);
    CFDictionarySetValue(matchDict, CFSTR(kIOHIDProductIDKey), pid);
    CFRelease(pid);

    //Usage
    val = MOOLTIPASS_USAGE;
    CFNumberRef usage = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &val);
    CFDictionarySetValue(matchDict, CFSTR(kIOHIDDeviceUsageKey), usage);
    CFRelease(usage);

    //Usage Page
    val = MOOLTIPASS_USAGE_PAGE;
    CFNumberRef usagep = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &val);
    CFDictionarySetValue(matchDict, CFSTR(kIOHIDDeviceUsagePageKey), usagep);
    CFRelease(usagep);

    // Register the Matching Dictionary to the HID Manager
    IOHIDManagerSetDeviceMatching(hidmanager, matchDict);

    // Register a callback for USB device detection with the HID Manager
    IOHIDManagerRegisterDeviceMatchingCallback(hidmanager, &_device_matching_callback, this);

    // Register a callback fro USB device removal with the HID Manager
    IOHIDManagerRegisterDeviceRemovalCallback(hidmanager, &_device_removal_callback, this);

    // Register the HID Manager on our app’s run loop
    // CFRunLoopGetCurrent() can safely be used because Qt uses CFRunloop in its backend platform plugin
    IOHIDManagerScheduleWithRunLoop(hidmanager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    // Open the HID Manager
    IOReturn ioret = IOHIDManagerOpen(hidmanager, kIOHIDOptionsTypeNone);
    if (ioret)
        qWarning() << "Failed to open IOHIDManager";
}

UsbMonitor_mac::~UsbMonitor_mac()
{
    IOHIDManagerClose(hidmanager, kIOHIDOptionsTypeNone);
    CFRelease(hidmanager);
}

QList<MPPlatformDef> UsbMonitor_mac::getDeviceList()
{
    QList<MPPlatformDef> deviceList;

    for (auto it = deviceHash.begin();it != deviceHash.end();it++)
        deviceList << it.value();

    return deviceList;
}
