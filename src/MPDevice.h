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
#ifndef MPDEVICE_H
#define MPDEVICE_H

#include <QObject>
#include "Common.h"
#include "MooltipassCmds.h"
#include "QtHelper.h"

typedef std::function<void(bool success, const QByteArray &data)> MPCommandCb;

class MPCommand
{
public:
    QByteArray data;
    MPCommandCb cb;
    bool running = false;
};

class MPDevice: public QObject
{
    Q_OBJECT
    QT_WRITABLE_PROPERTY(Common::MPStatus, status)
    QT_WRITABLE_PROPERTY(int, keyboardLayout)
    QT_WRITABLE_PROPERTY(bool, lockTimeoutEnabled)
    QT_WRITABLE_PROPERTY(int, lockTimeout)
    QT_WRITABLE_PROPERTY(bool, screensaver)
    QT_WRITABLE_PROPERTY(bool, userRequestCancel)
    QT_WRITABLE_PROPERTY(int, userInteractionTimeout)
    QT_WRITABLE_PROPERTY(bool, flashScreen)
    QT_WRITABLE_PROPERTY(bool, offlineMode)
    QT_WRITABLE_PROPERTY(bool, tutorialEnabled)

public:
    MPDevice(QObject *parent);
    virtual ~MPDevice();

    /* Send a command with data to the device */
    void sendData(unsigned char cmd, const QByteArray &data = QByteArray(), MPCommandCb cb = [](bool, const QByteArray &){});
    void sendData(unsigned char cmd, MPCommandCb cb = [](bool, const QByteArray &){});

signals:
    /* Signal emited by platform code when new data comes from MP */
    /* A signal is used for platform code that uses a dedicated thread */
    void platformDataRead(const QByteArray &data);

    /* the command has failed in platform code */
    void platformFailed();

private slots:
    void newDataRead(const QByteArray &data);
    void commandFailed();
    void sendDataDequeue();

private:
    /* Platform function for starting a read, should be implemented in platform class */
    virtual void platformRead() {}

    /* Platform function for writing data, should be implemented in platform class */
    virtual void platformWrite(const QByteArray &data) { Q_UNUSED(data); }

    //timer that asks status
    QTimer *statusTimer = nullptr;

    //command queue
    QQueue<MPCommand> commandQueue;

    //reload parameters from MP
    void loadParameters();
};

#endif // MPDEVICE_H
