#ifndef MPDEVICE_EMUL_H
#define MPDEVICE_EMUL_H
#include <QHash>
#include "MPDevice.h"

class MPDevice_emul : public MPDevice
{
public:
    MPDevice_emul(QObject *parent);
private:
    virtual void platformRead();
    virtual void platformWrite(const QByteArray &data);
    QHash<quint8, quint8> mooltipassParam;
};

#endif // MPDEVICE_EMUL_H
