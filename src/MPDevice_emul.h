#ifndef MPDEVICE_EMUL_H
#define MPDEVICE_EMUL_H

#include "MPDevice.h"

class MPDevice_emul : public MPDevice
{
public:
    MPDevice_emul(QObject *parent);
private:
    virtual void platformRead();
    virtual void platformWrite(const QByteArray &data);
};

#endif // MPDEVICE_EMUL_H
