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
#include "UsbMonitor_win.h"

UsbMonitor_win::UsbMonitor_win():
    QWidget()
{
    qDebug() << "Registering device events";

    DEV_BROADCAST_DEVICEINTERFACE db;
    db.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    db.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;

    notifyHandle = RegisterDeviceNotification((HWND)winId(),
                                              &db,
                                              DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
    if (!notifyHandle)
        qWarning() << "Unable to register for device notifications";
}

UsbMonitor_win::~UsbMonitor_win()
{
    if (!UnregisterDeviceNotification(notifyHandle))
        qWarning() << "Unable to unregister for device notifications";
}

bool UsbMonitor_win::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);

    MSG *msg = reinterpret_cast<MSG *>(message);
    if (!msg) return false;

    if (msg->message == WM_DEVICECHANGE && msg->lParam)
    {
        DEV_BROADCAST_DEVICEINTERFACE* db = reinterpret_cast<DEV_BROADCAST_DEVICEINTERFACE*>(msg->lParam);

        QString dev_path = QString::fromWCharArray(db->dbcc_name);
        QUuid guid(db->dbcc_classguid);

        if (msg->wParam == DBT_DEVICEARRIVAL)
        {
            qDebug() << "Device added: " << guid.toString() << " - " << dev_path;
            emit usbDeviceAdded(/*guid, dev_path*/);
        }
        else if (msg->wParam == DBT_DEVICEREMOVECOMPLETE)
        {
            qDebug() << "Device removed: " << guid.toString() << " - " << dev_path;
            emit usbDeviceRemoved(/*guid, dev_path*/);
        }
    }

    return false;
}
