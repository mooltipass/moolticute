#include "MPDevice_emul.h"
#include "MooltipassCmds.h"

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
        break;
    }
    case MP_START_MEMORYMGMT:
    {
        QByteArray d;
        d[0] = 1;
        d[1] = MP_START_MEMORYMGMT;
        d[2] = 0x01;
        emit platformDataRead(d);
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
    }
    default:
        emit platformDataRead(data);
        break;
    }

}

void MPDevice_emul::platformRead()
{
    qDebug() << "PlatformRead";
}
