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

#define CLEAN_MEMORY_QBYTEARRAY(d) do {for (int i = 0; i < d.size(); i++){d[i] = qrand() % 256;}} while(0);


MPDevice_emul::MPDevice_emul(QObject *parent):
    MPDevice(parent)
{
    qDebug() << "Emulation Device";

}

void MPDevice_emul::platformWrite(const QByteArray &data)
{
    switch((MPCmd::Command)data[1])
    {
    case MPCmd::PING:
        emit platformDataRead(data);
        break;
    case MPCmd::VERSION:
    {
        QByteArray d;
        d[1] = MPCmd::VERSION;
        d[2] = 0x08;
        d.append("v1.0_emul");
        d[0] = d.size() - 1;
        d.resize(64);
        emit platformDataRead(d);
        CLEAN_MEMORY_QBYTEARRAY(d);
        break;
    }
    case MPCmd::START_MEMORYMGMT:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = MPCmd::START_MEMORYMGMT;
        d[2] = 0x01;
        d.resize(64);
        emit platformDataRead(d);
        break;
    }
    case MPCmd::SET_MOOLTIPASS_PARM:
    {
        mooltipassParam[data[2]] = data [3];
        QByteArray d;
        d[0] = 1;
        d[1] = MPCmd::SET_MOOLTIPASS_PARM;
        d[2] = 0x01;
        d.resize(64);
        emit platformDataRead(d);
        break;
    }
    case MPCmd::GET_MOOLTIPASS_PARM:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = MPCmd::GET_MOOLTIPASS_PARM;
        d[2] = mooltipassParam[data[2]];
        d.resize(64);
        emit platformDataRead(d);
        break;
    }
    case MPCmd::MOOLTIPASS_STATUS:
    {
        QByteArray d;
        d[0] = 0x01;
        d[1] = MPCmd::MOOLTIPASS_STATUS;
        d[2] = 0b101;
        d.resize(64);
        emit platformDataRead(d);
        break;
    }
    case MPCmd::END_MEMORYMGMT:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = MPCmd::END_MEMORYMGMT;
        d[2] = 0x01;
        d.resize(64);
        emit platformDataRead(d);
        break;
    }
    case MPCmd::CONTEXT:
    {
        context = QString::fromUtf8(data.mid(2, data.length()));
        qDebug() << "Context : " << context;
        QByteArray d;
        d[0] = 1;
        d[1] = MPCmd::CONTEXT;
        d[2] = (quint8)logins.contains(context);
        d.resize(64);
        emit platformDataRead(d);
        break;
    }
    case MPCmd::GET_LOGIN:
    {
        QByteArray d;

        d[1] = MPCmd::GET_LOGIN;
        if (logins.contains(context))
            d.append(logins[context]);
        else
            d[2] = 0x0;
        d[0] = d.length() - 2;
        d.resize(64);
        emit platformDataRead(d);
        break;
    }
    case MPCmd::GET_PASSWORD:
    {
        QByteArray d;

        d[1] = MPCmd::GET_PASSWORD;
        if (passwords.contains(context))
            d.append(passwords[context]);
        else
            d[2] = 0x0;
        d[0] = d.length() - 2;
        d.resize(64);
        emit platformDataRead(d);
        break;
    }
    case MPCmd::ADD_CONTEXT:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = MPCmd::ADD_CONTEXT;

        context = QString::fromUtf8(data.mid(2, data.length()));
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
        emit platformDataRead(d);
        break;
    }
    case MPCmd::SET_LOGIN:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = MPCmd::SET_LOGIN;
        QString login = QString::fromUtf8(data.mid(2, data.length()));
        logins[context] = login;
        d[2] = 0x01;
        d.resize(64);
        emit platformDataRead(d);
        break;
    }
    case MPCmd::SET_PASSWORD:
    {
        QString password = QString::fromUtf8(data.mid(2, data.length()));
        passwords[context] = password;

        QByteArray d;
        d[0] = 1;
        d[1] = MPCmd::SET_PASSWORD;
        d[2] = 0x01;
        d.resize(64);
        emit platformDataRead(d);
        break;
    }
    case MPCmd::GET_RANDOM_NUMBER:
    {
         QByteArray d;
         d[0] = 32;
         d[1] = MPCmd::GET_RANDOM_NUMBER;
         for (int i = 0; i < 32; i++)
             d[2 + i] = qrand() % 255;
         d.resize(64);
         emit platformDataRead(d);
         break;
    }
    case MPCmd::GET_FAVORITE:
    {
        QByteArray d;
        d[0] = 4;
        d[1] = MPCmd::GET_FAVORITE;
        for (int i = 0; i < 4; i++)
            d[i] = 0x00;

        d.resize(64);
        emit platformDataRead(d);
        break;
    }
    case MPCmd::READ_FLASH_NODE:
    {
        QByteArray d(134, 0x00);
        d[0] = d.size() - 2;
        d[1] = MPCmd::READ_FLASH_NODE;
        d.resize(64);
        emit platformDataRead(d);
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
        qDebug() << "Unimplemented emulation command: " << data;
        QByteArray d = data;
        d.resize(64);
        d[2] = 0; //result is 0
        emit platformDataRead(d);
        break;
    }

}

void MPDevice_emul::platformRead()
{
    qDebug() << "PlatformRead";
}
