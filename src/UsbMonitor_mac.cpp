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

    int vendorID = 0;
    CFNumberRef vendorNumRef = (CFNumberRef)IOHIDDeviceGetProperty(inIOHIDDeviceRef, CFSTR(kIOHIDVendorIDKey)) ;
    if (vendorNumRef)
    {
        CFNumberGetValue(vendorNumRef, kCFNumberSInt32Type, &vendorID);
    }
    else
    {
        qWarning("VendorID is not found.");
    }

    MPPlatformDef def;
    def.hidref = inIOHIDDeviceRef;
    def.id = QString("%1").arg((quint64)def.hidref);
    def.isBLE = vendorID == MOOLTIPASS_BLE_VENDORID;

    if (!um->deviceHash.contains(def.id))
    {
        um->deviceHash[def.id] = def;
        emit um->usbDeviceAdded(def.id);
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

    QString hid_id = QString("%1").arg((quint64)inIOHIDDeviceRef);
    if (um->deviceHash.contains(hid_id))
    {
        MPPlatformDef def = um->deviceHash[hid_id];
        um->deviceHash.remove(hid_id);
        emit um->usbDeviceRemoved(hid_id);
        qDebug() << "Device removed: " << def.id;
    }
}

UsbMonitor_mac::UsbMonitor_mac():
    QObject()
{
    // Init HID Manager
    initHidmanager(hidmanager, MOOLTIPASS_VENDORID, MOOLTIPASS_PRODUCTID);
    // Init HID Manager for BLE
    initHidmanager(hidmanagerBLE, MOOLTIPASS_BLE_VENDORID, MOOLTIPASS_BLE_PRODUCTID);
}

void UsbMonitor_mac::initHidmanager(IOHIDManagerRef &hidMan, int vendorId, int productId)
{
    // Create an HID Manager
    hidMan = IOHIDManagerCreate(kCFAllocatorDefault,
                                                    kIOHIDOptionsTypeNone);

    // Create a Matching Dictionary for filtering HID devices
    CFMutableDictionaryRef matchDict = CFDictionaryCreateMutable(kCFAllocatorDefault,
                                                                 4,
                                                                 &kCFTypeDictionaryKeyCallBacks,
                                                                 &kCFTypeDictionaryValueCallBacks);

    // Set device info in the Matching Dictionary (pid/vid/usage/usagepage)

    //VendorID
    int val = vendorId;
    CFNumberRef vid = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &val);
    CFDictionarySetValue(matchDict, CFSTR(kIOHIDVendorIDKey), vid);
    CFRelease(vid);

    //ProductID
    val = productId;
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
    IOHIDManagerSetDeviceMatching(hidMan, matchDict);

    // Register a callback for USB device detection with the HID Manager
    IOHIDManagerRegisterDeviceMatchingCallback(hidMan, &_device_matching_callback, this);

    // Register a callback fro USB device removal with the HID Manager
    IOHIDManagerRegisterDeviceRemovalCallback(hidMan, &_device_removal_callback, this);

    // Register the HID Manager on our app’s run loop
    // CFRunLoopGetCurrent() can safely be used because Qt uses CFRunloop in its backend platform plugin
    IOHIDManagerScheduleWithRunLoop(hidMan, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

    // Open the HID Manager
    IOReturn ioret = IOHIDManagerOpen(hidMan, kIOHIDOptionsTypeNone);
    if (ioret)
        qWarning() << "Failed to open IOHIDManager";
}

UsbMonitor_mac::~UsbMonitor_mac()
{
    IOHIDManagerClose(hidmanager, kIOHIDOptionsTypeNone);
    CFRelease(hidmanager);
    IOHIDManagerClose(hidmanagerBLE, kIOHIDOptionsTypeNone);
    CFRelease(hidmanagerBLE);
}

QList<MPPlatformDef> UsbMonitor_mac::getDeviceList()
{
    QList<MPPlatformDef> deviceList;

    for (auto it = deviceHash.begin();it != deviceHash.end();it++)
        deviceList << it.value();

    return deviceList;
}
