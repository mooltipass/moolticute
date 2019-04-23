#ifndef MPNODEBLEIMPL_H
#define MPNODEBLEIMPL_H

#include "MPNodeImpl.h"

class MPNodeBLEImpl : public MPNodeImpl
{
public:
    MPNodeBLEImpl();
    bool isValid(QByteArray data) override;
};

#endif // MPNODEBLEIMPL_H
