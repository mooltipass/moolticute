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
#include "MPDevice_emul.h"
#include "MooltipassCmds.h"

#define CLEAN_MEMORY_QBYTEARRAY(d) do {for (int i = 0; i < d.size(); i++){d[i] = Common::getRand() % 256;}} while(0);


MPDevice_emul::MPDevice_emul(QObject *parent):
    MPDevice(parent)
{
    qDebug() << "Emulation Device";
    setupMessageProtocol();
    sendInitMessages();
}

void MPDevice_emul::platformWrite(const QByteArray &data)
{
    qDebug() << "Sending into emu" << MPCmd::printCmd(data) << data.mid(2);

    const char commandData = static_cast<char>(data[1]);
    MPCmd::Command cmd = pMesProt->getGeneralCommandId(static_cast<quint8>(commandData));
    switch(cmd)
    {
    case MPCmd::PING:
        sendReadSignal(data);
        break;
    case MPCmd::VERSION:
    {
        QByteArray d;
        d[1] = commandData;
        d[2] = 0x08;
        d.append("v1.0_emul");
        d[0] = d.size() - 1;
        d.resize(64);
        sendReadSignal(d);
        CLEAN_MEMORY_QBYTEARRAY(d);
        break;
    }
    case MPCmd::START_MEMORYMGMT:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = commandData;
        d[2] = 0x01;
        d.resize(64);
        sendReadSignal(d);
        break;
    }
    case MPCmd::SET_MOOLTIPASS_PARM:
    {
        mooltipassParam[data[2]] = data [3];
        QByteArray d;
        d[0] = 1;
        d[1] = commandData;
        d[2] = 0x01;
        d.resize(64);
        sendReadSignal(d);
        break;
    }
    case MPCmd::GET_MOOLTIPASS_PARM:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = commandData;
        d[2] = mooltipassParam[data[2]];
        d.resize(64);
        sendReadSignal(d);
        break;
    }
    case MPCmd::MOOLTIPASS_STATUS:
    {
        QByteArray d;
        d[0] = 0x01;
        d[1] = commandData;
        d[2] = 0b101;
        d.resize(64);
        sendReadSignal(d);
        break;
    }
    case MPCmd::END_MEMORYMGMT:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = commandData;
        d[2] = 0x01;
        d.resize(64);
        sendReadSignal(d);
        break;
    }
    case MPCmd::CONTEXT:
    {
        context = pMesProt->toQString(data.mid(2, data.length()));
        qDebug() << "Context : " << context;
        QByteArray d;
        d[0] = 1;
        d[1] = commandData;
        d[2] = (quint8)logins.contains(context);
        d.resize(64);
        sendReadSignal(d);
        break;
    }
    case MPCmd::GET_LOGIN:
    {
        QByteArray d;

        d[1] = commandData;
        if (logins.contains(context))
            d.append(logins[context].toUtf8());
        else
            d[2] = 0x0;
        d[0] = d.length() - 2;
        d.resize(64);
        sendReadSignal(d);
        break;
    }
    case MPCmd::GET_PASSWORD:
    {
        QByteArray d;

        d[1] = commandData;
        if (passwords.contains(context))
            d.append(passwords[context].toUtf8());
        else
            d[2] = 0x0;
        d[0] = d.length() - 2;
        d.resize(64);
        sendReadSignal(d);
        break;
    }
    case MPCmd::ADD_CONTEXT:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = commandData;

        context = pMesProt->toQString(data.mid(2, data.length()));
        qDebug() << "Context : " << context;
        if (!passwords.contains(context))
        {
            passwords[context] = "";
            logins[context] = "";
            d[2] = 0x1;
        }
        else
        {
            d[2] = 0x00;
        }
        d.resize(64);
        sendReadSignal(d);
        break;
    }
    case MPCmd::SET_LOGIN:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = commandData;
        QString login = pMesProt->toQString(data.mid(2, data.length()));
        logins[context] = login;
        d[2] = 0x01;
        d.resize(64);
        sendReadSignal(d);
        break;
    }
    case MPCmd::SET_PASSWORD:
    {
        QString password = pMesProt->toQString(data.mid(2, data.length()));
        passwords[context] = password;

        QByteArray d;
        d[0] = 1;
        d[1] = commandData;
        d[2] = 0x01;
        d.resize(64);
        sendReadSignal(d);
        break;
    }
    case MPCmd::CHECK_PASSWORD: /* 0xA8 */
    {
        /* Currently we do nothing (only send back 0x01) */
        QByteArray d;
        d[0] = 1;
        d[1] = commandData;
        d[2] = 0x00;

        d.resize(64);
        sendReadSignal(d);
        break;
    }
    case MPCmd::GET_RANDOM_NUMBER: /* 0xAC */
    {
         QByteArray d;
         d[0] = 32;
         d[1] = commandData;
         for (int i = 0; i < 32; i++)
             d[2 + i] = Common::getRand() % 255;
         d.resize(64);
         sendReadSignal(d);
         break;
    }
    case MPCmd::SET_DATE: /* 0xBB */
    {
        /* Currently we do nothing (only send back 0x01) */
        QByteArray d;
        d[0] = 1;
        d[1] = commandData;
        d[2] = 0x01;

        d.resize(64);
        sendReadSignal(d);
        break;
    }
    case MPCmd::GET_CUR_CARD_CPZ: /* 0xC2 */
    {
        QByteArray d;
        d[0] = 8;
        d[1] = commandData;
        for (int i = 2; i < 10; i++)
            d[i] = 0xFF;

        d.resize(64);
        sendReadSignal(d);
        break;
    }
    case MPCmd::READ_FLASH_NODE: /* 0xC5 */
    {
        QByteArray d(134, 0x00);
        d[0] = d.size() - 2;
        d[1] = commandData;
        d.resize(64);
        sendReadSignal(d);
        break;
    }
    case MPCmd::GET_FAVORITE: /* 0xC7 */
    {
        QByteArray d;
        d[0] = 4;
        d[1] = commandData;
        for (int i = 2; i < 6; i++)
            d[i] = 0x00;

        d.resize(64);
        sendReadSignal(d);
        break;
    }
      /* MPCmd::READ_FLASH_NODE
               MPCmd::GET_DN_START_PARENT
               MPCmd::GET_STARTING_PARENT
               MPCmd::GET_FAVORITE
               MPCmd::GET_CARD_CPZ_CTR
               MPCmd::GET_CTRVALUE
               MPCmd::GET_RANDOM_NUMBER*/
    default:
        qDebug() << "Unimplemented emulation command: " << MPCmd::printCmd(data) << data.mid(2);
        QByteArray d = data;
        d.resize(64);
        d[2] = 0; //result is 0
        sendReadSignal(d);
        break;
    }
}

void MPDevice_emul::sendReadSignal(const QByteArray &data)
{
    QTimer::singleShot(0, [=]()
    {
        emit platformDataRead(data);
    });
}

void MPDevice_emul::platformRead()
{
    qDebug() << "PlatformRead";
}
