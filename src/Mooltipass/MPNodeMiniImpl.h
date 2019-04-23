#ifndef MPNODEMINIIMPL_H
#define MPNODEMINIIMPL_H

#include "MPNodeImpl.h"

class MPNodeMiniImpl : public MPNodeImpl
{
public:
    MPNodeMiniImpl();
    bool isValid(QByteArray data) override;
};

#endif // MPNODEMINIIMPL_H
