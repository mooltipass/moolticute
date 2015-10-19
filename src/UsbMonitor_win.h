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
#ifndef USBMONITOR_WIN_H
#define USBMONITOR_WIN_H

#include <QtCore>
#include <QWidget>
#include <qt_windows.h>
#include <dbt.h>
#include <windows.h>

class UsbMonitor_win: public QWidget
{
    Q_OBJECT
public:
    static UsbMonitor_win *Instance()
    {
        static UsbMonitor_win inst;
        return &inst;
    }
    ~UsbMonitor_win();

signals:
    void usbDeviceAdded();
    void usbDeviceRemoved();

private:
    UsbMonitor_win();

    HDEVNOTIFY notifyHandle = nullptr;

    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result);
};

#endif // USBMONITOR_WIN_H
