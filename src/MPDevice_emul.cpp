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
    m_defaultCPZ = "123456";
}

void MPDevice_emul::platformWrite(const QByteArray &data)
{
    m_data.clear();
    m_data.resize(64);
    m_data[0] = 0x00;

    switch(MPCmd::from(data[MP_CMD_FIELD_INDEX]))
    {
    case MPCmd::PING:
        m_data = data;
        break;
    case MPCmd::VERSION:
    {
        QByteArray version("v1.0_emul");
        m_data[1] = MPCmd::VERSION;
        m_data[2] = 0x08;
        m_data.replace(3, version.size(), version);
        m_data[0] = version.size();
        break;
    }
    case MPCmd::START_MEMORYMGMT:
    {
        m_data[0] = 1;
        m_data[1] = MPCmd::START_MEMORYMGMT;
        m_data[2] = 0x01;
        break;
    }
    case MPCmd::SET_MOOLTIPASS_PARM:
    {
        mooltipassParam[data[2]] = data[3];
        m_data[0] = 1;
        m_data[1] = MPCmd::SET_MOOLTIPASS_PARM;
        m_data[2] = 0x01;
        break;
    }
    case MPCmd::GET_MOOLTIPASS_PARM:
    {
        m_data[0] = 1;
        m_data[1] = MPCmd::GET_MOOLTIPASS_PARM;
        m_data[2] = mooltipassParam[data[2]];
        break;
    }
    case MPCmd::MOOLTIPASS_STATUS:
    {
        m_data[0] = 0x01;
        m_data[1] = MPCmd::MOOLTIPASS_STATUS;
        m_data[2] = 0b101;
        break;
    }
    case MPCmd::END_MEMORYMGMT:
    {
        m_data[0] = 1;
        m_data[1] = MPCmd::END_MEMORYMGMT;
        m_data[2] = 0x01;
        break;
    }
    case MPCmd::CONTEXT:
    {
        context = QString::fromUtf8(data.mid(2, data.length()));
        qDebug() << "Context : " << context;
        m_data[0] = 1;
        m_data[1] = MPCmd::CONTEXT;
        m_data[2] = (quint8)logins.contains(context);
        break;
    }
    case MPCmd::GET_LOGIN:
    {
        m_data[1] = MPCmd::GET_LOGIN;
        QString login = logins[context];
        if (!login.isEmpty())
            m_data.replace(2, login.size(), login.toLocal8Bit());
        else
            m_data[2] = 0x0;
        m_data[0] = login.size();
        break;
    }
    case MPCmd::GET_PASSWORD:
    {
        m_data[1] = MPCmd::GET_PASSWORD;
        QString password = passwords[context];
        if (!password.isEmpty())
            m_data.replace(2, password.size(), password.toLocal8Bit());
        else
            m_data[2] = 0x0;
        m_data[0] = password.size();
        break;
    }
    case MPCmd::ADD_CONTEXT:
    {
        m_data[0] = 1;
        m_data[1] = MPCmd::ADD_CONTEXT;

        context = QString::fromUtf8(data.mid(2, data.length()));
        qDebug() << "Context : " << context;
        if (!passwords.contains(context))
        {
            passwords[context] = "";
            logins[context] = "";
            m_data[2] = 0x1;
        }
        else
        {
            m_data[2] = 0x00;
        }
        break;
    }
    case MPCmd::SET_LOGIN:
    {
        m_data[0] = 1;
        m_data[1] = MPCmd::SET_LOGIN;
        QString login = QString::fromUtf8(data.mid(2, data.length()));
        logins[context] = login;
        m_data[2] = 0x01;
        break;
    }
    case MPCmd::SET_PASSWORD:
    {
        QString password = QString::fromUtf8(data.mid(2, data.length()));
        passwords[context] = password;

        m_data[0] = 1;
        m_data[1] = MPCmd::SET_PASSWORD;
        m_data[2] = 0x01;
        break;
    }
    case MPCmd::GET_RANDOM_NUMBER:
    {
        m_data[0] = 32;
        m_data[1] = MPCmd::GET_RANDOM_NUMBER;
        for (int i = 0; i < 32; i++)
            m_data[2 + i] = qrand() % 255;
        break;
    }
    case MPCmd::GET_FAVORITE:
    {
        m_data[0] = 4;
        m_data[1] = MPCmd::GET_FAVORITE;
        for (int i = 0; i < 4; i++)
            m_data[i] = 0x00;
        break;
    }
    case MPCmd::READ_FLASH_NODE:
    {
        m_data.resize(134);
        m_data.fill(0x00);

        m_data[0] = m_data.size() - 2;
        m_data[1] = MPCmd::READ_FLASH_NODE;
        break;
    }
    case MPCmd::GET_CUR_CARD_CPZ:
    {
        m_data[1] = MPCmd::GET_CUR_CARD_CPZ;
        m_data.replace(2, m_defaultCPZ.size(), m_defaultCPZ);
        m_data[0] = m_defaultCPZ.size();
        break;
    }
    case MPCmd::SET_DATE:
    {
        m_data[1] = MPCmd::SET_DATE;
        int dataSize = data.size() - 2;
        m_data.replace(2, dataSize, data.mid(2));
        m_data[0] = dataSize;
        break;
    }
        /* MPCmd::READ_FLASH_NODE
               MPCmd::GET_DN_START_PARENT
               MPCmd::GET_STARTING_PARENT
               MPCmd::GET_FAVORITE
               MPCmd::GET_CTRVALUE
               MPCmd::GET_RANDOM_NUMBER*/
    default:
        qDebug() << "Unimplemented emulation command: " << data;
        m_data[2] = 0; //result is 0
        m_data.replace(3, data.size(), data);
        m_data[0] = data.size();
        break;
    }

    if(m_data.at(0))
    {
        QTimer::singleShot(0, [=]()
        {
            emit platformDataRead(m_data);
        });
    }
}

void MPDevice_emul::platformRead()
{
    qDebug() << "PlatformRead";
}
