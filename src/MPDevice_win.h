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
#ifndef MPDEVICE_WIN_H
#define MPDEVICE_WIN_H

#include "MPDevice.h"

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
#include <qt_windows.h>
#include <winbase.h>
#include <setupapi.h>
#include <winioctl.h>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
#include "qwinoverlappedionotifier.h"
#else
#include <private/qwinoverlappedionotifier_p.h>
#endif

#include "MPDevice.h"

class MPPlatformDef
{
public:
    QString id; //unique id for all platform

    HANDLE devHandle = INVALID_HANDLE_VALUE;
    bool pendingRead = false;
    QString path;
    USHORT outReportLen = 0;
    USHORT inReportLen = 0;
    bool isBLE = false;
};

inline bool operator==(const MPPlatformDef &lhs, const MPPlatformDef &rhs) { return lhs.id == rhs.id; }
inline bool operator!=(const MPPlatformDef &lhs, const MPPlatformDef &rhs) { return !(lhs == rhs); }

class MPDevice_win: public MPDevice
{
    Q_OBJECT
public:
    MPDevice_win(QObject *parent, const MPPlatformDef &platformDef);
    virtual ~MPDevice_win();

    //Static function for enumerating devices on platform
    static QList<MPPlatformDef> enumerateDevices();
    /**
     * @brief checkDevice
     * Checking if the device is a mooltipass device
     * @param path to the device
     * @param isBLE out param, true if device is a ble
     * @return true, if the device is mini/ble
     */
    static bool checkDevice(QString path, bool &isBLE);
    static MPPlatformDef getPlatDef(QString path, bool isBLE);

signals:
    void platformWriteFinished();

private slots:
    void ovlpNotified(quint32 numberOfBytes, quint32 errorCode, OVERLAPPED *overlapped);
    void writeDataFinished();

private:
    virtual void platformRead();
    virtual void platformWrite(const QByteArray &data);
    void platformWriteToDevice(const QByteArray &data);
    void addToWriteQueue(const QByteArray& data);

    bool openPath();
    static HANDLE openDevice(QString path, bool exlusive = false);

    //GetLastError() helper
    QString getLastError(DWORD err);

    MPPlatformDef platformDef;

    QByteArray readBuffer;
    OVERLAPPED writeOverlapped;
    OVERLAPPED readOverlapped;
    QWinOverlappedIoNotifier *oNotifier;

    QQueue<QByteArray> m_writeQueue;
};

#endif // MPDEVICE_WIN_H
