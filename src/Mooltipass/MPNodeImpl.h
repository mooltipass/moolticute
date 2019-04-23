#ifndef MPNODEIMPL_H
#define MPNODEIMPL_H

#include <QByteArray>

class MPNodeImpl
{
public:
    MPNodeImpl();
    virtual bool isValid(QByteArray data) = 0;
};

#endif // MPNODEIMPL_H
