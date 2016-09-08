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
    qDebug() << "PlatformWrite : " << data;

    switch((quint8)data[1])
    {
    case MP_PING:
        emit platformDataRead(data);
        break;
    case MP_VERSION:
    {
        QByteArray d;
        d[1] = MP_VERSION;
        d.append("EMULATION_VERSION");
        d[0] = d.size() - 2;
        emit platformDataRead(d);
        CLEAN_MEMORY_QBYTEARRAY(d);
        break;
    }
    case MP_START_MEMORYMGMT:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = MP_START_MEMORYMGMT;
        d[2] = 0x01;
        emit platformDataRead(d);
        break;
    }
    case MP_SET_MOOLTIPASS_PARM:
    {
        mooltipassParam[data[2]] = data [3];
        QByteArray d;
        d[0] = 1;
        d[1] = MP_SET_MOOLTIPASS_PARM;
        d[2] = 0x01;
        emit platformDataRead(d);
        break;
    }
    case MP_GET_MOOLTIPASS_PARM:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = MP_SET_MOOLTIPASS_PARM;
        d[2] = mooltipassParam[data[2]];
        emit platformDataRead(d);
        break;
    }
    case MP_MOOLTIPASS_STATUS:
    {
        QByteArray d;
        d[0] = 0x01;
        d[1] = MP_MOOLTIPASS_STATUS;
        d[2] = 0b101;
        emit platformDataRead(d);
        break;
    }
    case MP_END_MEMORYMGMT:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = MP_END_MEMORYMGMT;
        d[2] = 0x01;
        emit platformDataRead(d);
        break;
    }
    case MP_CONTEXT:
    {
        context = QString::fromUtf8(data.mid(2, data.length()));
        qDebug() << "Context : " << context;
        QByteArray d;
        d[0] = 1;
        d[1] = MP_CONTEXT;
        d[2] = (quint8)logins.contains(context);
        emit platformDataRead(d);
        break;
    }
    case MP_GET_LOGIN:
    {
        QByteArray d;

        d[1] = MP_GET_LOGIN;
        if (logins.contains(context))
            d.append(logins[context]);
        else
            d[2] = 0x0;
        d[0] = d.length() - 2;
        emit platformDataRead(d);
        break;
    }
    case MP_GET_PASSWORD:
    {
        QByteArray d;

        d[1] = MP_GET_PASSWORD;
        if (passwords.contains(context))
            d.append(passwords[context]);
        else
            d[2] = 0x0;
        d[0] = d.length() - 2;
        emit platformDataRead(d);
        break;
    }
    case MP_ADD_CONTEXT:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = MP_ADD_CONTEXT;

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
        emit platformDataRead(d);
        break;
    }
    case MP_SET_LOGIN:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = MP_SET_LOGIN;
        QString login = QString::fromUtf8(data.mid(2, data.length()));
        logins[context] = login;
        d[2] = 0x01;
        emit platformDataRead(d);
        break;
    }
    case MP_SET_PASSWORD:
    {
        QString password = QString::fromUtf8(data.mid(2, data.length()));
        passwords[context] = password;

        QByteArray d;
        d[0] = 1;
        d[1] = MP_SET_PASSWORD;
        d[2] = 0x01;
        emit platformDataRead(d);
        break;
    }
    case MP_GET_RANDOM_NUMBER:
    {
         QByteArray d;
         d[0] = 32;
         d[1] = MP_GET_RANDOM_NUMBER;
         for (int i = 0; i < 32; i++)
             d[2 + i] = qrand() % 255;
         emit platformDataRead(d);
         break;
    }
    case MP_GET_FAVORITE:
    {
        QByteArray d;
        d[0] = 4;
        d[1] = MP_GET_FAVORITE;
        for (int i = 0; i < 4; i++)
            d[i] = 0x00;

        emit platformDataRead(d);
        break;
    }
    case MP_READ_FLASH_NODE:
    {
        QByteArray d(134, 0x00);
        d[0] = d.size() - 2;
        d[1] = MP_READ_FLASH_NODE;
        emit platformDataRead(d);
        break;
    }
      /* MP_READ_FLASH_NODE
               MP_GET_DN_START_PARENT
               MP_GET_STARTING_PARENT
               MP_GET_FAVORITE
               MP_GET_CARD_CPZ_CTR
               MP_GET_CTRVALUE
               MP_GET_RANDOM_NUMBER*/
    default:
        emit platformDataRead(data);
        break;
    }

}

void MPDevice_emul::platformRead()
{
    qDebug() << "PlatformRead";
}
