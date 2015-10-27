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
        sendData(MP_MOOLTIPASS_STATUS, [=](bool success, const QByteArray &data)
        {
            if (!success)
                return;
            if ((quint8)data.at(1) == MP_MOOLTIPASS_STATUS)
            {
                Common::MPStatus s = (Common::MPStatus)data.at(2);
                qDebug() << "received MP_MOOLTIPASS_STATUS: " << (int)data.at(2);
                if (s != get_status())
                {
                    if (s == Common::Unlocked)
                        QTimer::singleShot(10, [=]() { loadParameters(); });
                }
                set_status(s);
            }
        });
    });

    connect(this, SIGNAL(platformDataRead(QByteArray)), this, SLOT(newDataRead(QByteArray)));
//    connect(this, SIGNAL(platformFailed()), this, SLOT(commandFailed()));
}

MPDevice::~MPDevice()
{
}

void MPDevice::sendData(unsigned char c, const QByteArray &data, MPCommandCb cb)
{
    MPCommand cmd;

    // Prepare MP packet
    cmd.data.append(data.size());
    cmd.data.append(c);
    cmd.data.append(data);
    cmd.cb = std::move(cb);

    commandQueue.enqueue(cmd);

    if (!commandQueue.head().running)
        sendDataDequeue();
}

void MPDevice::sendData(unsigned char cmd, MPCommandCb cb)
{
    sendData(cmd, QByteArray(), std::move(cb));
}

void MPDevice::sendDataDequeue()
{
    if (commandQueue.isEmpty())
        return;

    MPCommand &currentCmd = commandQueue.head();
    currentCmd.running = true;

    // send data with platform code
    qDebug() << "Platform send command: " << QString("0x%1").arg((quint8)currentCmd.data[1], 2, 16, QChar('0'));
    platformWrite(currentCmd.data);
}

void MPDevice::loadParameters()
{
    QByteArray ba;
    ba.append((char)KEYBOARD_LAYOUT_PARAM);
    sendData(MP_GET_MOOLTIPASS_PARM, ba, [=](bool success, const QByteArray &data)
    {
        if (!success) return;
        qDebug() << "received language: " << (quint8)data.at(2);
        set_keyboardLayout((quint8)data.at(2));
    });

    ba[0] = (char)LOCK_TIMEOUT_ENABLE_PARAM;
    sendData(MP_GET_MOOLTIPASS_PARM, ba, [=](bool success, const QByteArray &data)
    {
        if (!success) return;
        qDebug() << "received lock timeout enable: " << (quint8)data.at(2);
        set_lockTimeoutEnabled(data.at(2) != 0);
    });

    ba[0] = (char)LOCK_TIMEOUT_PARAM;
    sendData(MP_GET_MOOLTIPASS_PARM, ba, [=](bool success, const QByteArray &data)
    {
        if (!success) return;
        qDebug() << "received lock timeout: " << (quint8)data.at(2);
        set_lockTimeout((quint8)data.at(2));
    });

    ba[0] = (char)SCREENSAVER_PARAM;
    sendData(MP_GET_MOOLTIPASS_PARM, ba, [=](bool success, const QByteArray &data)
    {
        if (!success) return;
        qDebug() << "received screensaver: " << (quint8)data.at(2);
        set_screensaver(data.at(2) != 0);
    });

    ba[0] = (char)USER_REQ_CANCEL_PARAM;
    sendData(MP_GET_MOOLTIPASS_PARM, ba, [=](bool success, const QByteArray &data)
    {
        if (!success) return;
        qDebug() << "received userRequestCancel: " << (quint8)data.at(2);
        set_userRequestCancel(data.at(2) != 0);
    });

    ba[0] = (char)USER_INTER_TIMEOUT_PARAM;
    sendData(MP_GET_MOOLTIPASS_PARM, ba, [=](bool success, const QByteArray &data)
    {
        if (!success) return;
        qDebug() << "received userInteractionTimeout: " << (quint8)data.at(2);
        set_userInteractionTimeout((quint8)data.at(2));
    });

    ba[0] = (char)FLASH_SCREEN_PARAM;
    sendData(MP_GET_MOOLTIPASS_PARM, ba, [=](bool success, const QByteArray &data)
    {
        if (!success) return;
        qDebug() << "received flashScreen: " << (quint8)data.at(2);
        set_flashScreen(data.at(2) != 0);
    });

    ba[0] = (char)OFFLINE_MODE_PARAM;
    sendData(MP_GET_MOOLTIPASS_PARM, ba, [=](bool success, const QByteArray &data)
    {
        if (!success) return;
        qDebug() << "received offlineMode: " << (quint8)data.at(2);
        set_offlineMode(data.at(2) != 0);
    });

    ba[0] = (char)TUTORIAL_BOOL_PARAM;
    sendData(MP_GET_MOOLTIPASS_PARM, ba, [=](bool success, const QByteArray &data)
    {
        if (!success) return;
        qDebug() << "received tutorialEnabled: " << (quint8)data.at(2);
        set_tutorialEnabled(data.at(2) != 0);
    });
}

void MPDevice::commandFailed()
{
//    MPCommand currentCmd = commandQueue.head();
//    currentCmd.cb(false, QByteArray());
//    commandQueue.dequeue();

//    QTimer::singleShot(150, this, SLOT(sendDataDequeue()));
}

void MPDevice::newDataRead(const QByteArray &data)
{
    //we assume that the QByteArray size is at least 64 bytes
    //this should be done by the platform code

    MPCommand currentCmd = commandQueue.head();
    currentCmd.cb(true, data);
    commandQueue.dequeue();

    sendDataDequeue();
}

void MPDevice::updateKeyboardLayout(int lang)
{
    QByteArray ba;
    ba.append((quint8)KEYBOARD_LAYOUT_PARAM);
    ba.append((quint8)lang);
    sendData(MP_SET_MOOLTIPASS_PARM, ba);
}

void MPDevice::updateLockTimeoutEnabled(bool en)
{
    QByteArray ba;
    ba.append((quint8)LOCK_TIMEOUT_ENABLE_PARAM);
    ba.append((quint8)en);
    sendData(MP_SET_MOOLTIPASS_PARM, ba);
}

void MPDevice::updateLockTimeout(int timeout)
{
    if (timeout < 0) timeout = 0;
    if (timeout > 0xFF) timeout = 0xFF;

    QByteArray ba;
    ba.append((quint8)LOCK_TIMEOUT_PARAM);
    ba.append((quint8)timeout);
    sendData(MP_SET_MOOLTIPASS_PARM, ba);
}

void MPDevice::updateScreensaver(bool en)
{
    QByteArray ba;
    ba.append((quint8)SCREENSAVER_PARAM);
    ba.append((quint8)en);
    sendData(MP_SET_MOOLTIPASS_PARM, ba);
}

void MPDevice::updateUserRequestCancel(bool en)
{
    QByteArray ba;
    ba.append((quint8)USER_REQ_CANCEL_PARAM);
    ba.append((quint8)en);
    sendData(MP_SET_MOOLTIPASS_PARM, ba);
}

void MPDevice::updateUserInteractionTimeout(int timeout)
{
    if (timeout < 0) timeout = 0;
    if (timeout > 0xFF) timeout = 0xFF;

    QByteArray ba;
    ba.append((quint8)USER_INTER_TIMEOUT_PARAM);
    ba.append((quint8)timeout);
    sendData(MP_SET_MOOLTIPASS_PARM, ba);
}

void MPDevice::updateFlashScreen(bool en)
{
    QByteArray ba;
    ba.append((quint8)FLASH_SCREEN_PARAM);
    ba.append((quint8)en);
    sendData(MP_SET_MOOLTIPASS_PARM, ba);
}

void MPDevice::updateOfflineMode(bool en)
{
    QByteArray ba;
    ba.append((quint8)OFFLINE_MODE_PARAM);
    ba.append((quint8)en);
    sendData(MP_SET_MOOLTIPASS_PARM, ba);
}

void MPDevice::updateTutorialEnabled(bool en)
{
    QByteArray ba;
    ba.append((quint8)TUTORIAL_BOOL_PARAM);
    ba.append((quint8)en);
    sendData(MP_SET_MOOLTIPASS_PARM, ba);
}
