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
#include "MPDevice.h"

MPDevice::MPDevice(QObject *parent):
    QObject(parent)
{
    set_status(Common::UnknownStatus);

    statusTimer = new QTimer(this);
    statusTimer->start(500);
    connect(statusTimer, &QTimer::timeout, [=]()
    {
        sendData(MP_MOOLTIPASS_STATUS);
    });

    connect(this, SIGNAL(platformDataRead(QByteArray)), this, SLOT(newDataRead(QByteArray)));
}

MPDevice::~MPDevice()
{
}

void MPDevice::sendData(unsigned char cmd, const QByteArray &data)
{
    // Prepare MP packet
    qDebug() << "Send command: " << QString("0x%1").arg(cmd, 2, 16, QChar('0'));
    QByteArray ba;
    ba.append(data.size());
    ba.append(cmd);
    ba.append(data);

    // send data with platform code
    platformWrite(ba);
}

void MPDevice::newDataRead(const QByteArray &data)
{
    //we assume that the QByteArray size is at least 64 bytes
    //this should be done by the platform code

    switch ((unsigned char)data.at(1))
    {
    case MP_MOOLTIPASS_STATUS:
        qDebug() << "received MP_MOOLTIPASS_STATUS: " << (int)data.at(2);
        set_status((Common::MPStatus)data.at(2));
        break;

    default: break;
    }
}
