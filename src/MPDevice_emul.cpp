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

    switch((unsigned char)data[1])
    {
    case MP_PING:
        emit platformDataRead(data);
        break;
    case MP_VERSION:
    {

        break;
    }
    case MP_SET_MOOLTIPASS_PARM:
    {
        break;
    }
    case MP_MOOLTIPASS_STATUS:
    {
        QByteArray d = data;
        d[0] = 0x00;
        d[2] = 0b101;
        emit platformDataRead(d);
        break;
    }
    default:
        break;
    }

}

void MPDevice_emul::platformRead()
{
    qDebug() << "PlatformRead";
}
