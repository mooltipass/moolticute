#include "MPNodeBLEImpl.h"

#include <QDebug>

MPNodeBLEImpl::MPNodeBLEImpl(const QByteArray &d)
    : MPNodeImpl(d)
{

}

bool MPNodeBLEImpl::isValid()
{
    return (data.size() == PARENT_NODE_LENGTH || data.size() == CHILD_NODE_LENGTH);
}

bool MPNodeBLEImpl::isDataLengthValid()
{
    return isValid();
}

QByteArray MPNodeBLEImpl::getService() const
{
    return data.mid(SERVICE_ADDR_START, SERVICE_LENGTH);
}

void MPNodeBLEImpl::setService(QByteArray service)
{
    data.replace(SERVICE_ADDR_START, SERVICE_LENGTH, service);
}

QByteArray MPNodeBLEImpl::getStartDataCtr() const
{
    return data.mid(CTR_DATA_ADDR_START, CTR_DATA_LENGTH);
}

QByteArray MPNodeBLEImpl::getCTR() const
{
    return data.mid(CTR_ADDR_START, CTR_DATA_LENGTH);
}

QByteArray MPNodeBLEImpl::getDescription() const
{
    return data.mid(DESC_ADDR_START, DESC_LENGTH);
}

void MPNodeBLEImpl::setDescription(const QByteArray &desc)
{
    data.replace(DESC_ADDR_START, DESC_LENGTH, desc);
}

QByteArray MPNodeBLEImpl::getLogin() const
{
    return data.mid(LOGIN_ADDR_START, LOGIN_LENGTH);
}

void MPNodeBLEImpl::setLogin(const QByteArray &login)
{
    data.replace(LOGIN_ADDR_START, LOGIN_LENGTH, login);
}

QByteArray MPNodeBLEImpl::getPasswordEnc() const
{
    return data.mid(PWD_ENC_ADDR_START, PWD_ENC_LENGTH);
}

QByteArray MPNodeBLEImpl::getDateCreated() const
{
    return data.mid(DATE_CREATED_ADDR_START, ADDRESS_LENGTH);
}

QByteArray MPNodeBLEImpl::getDateLastUsed() const
{
    return data.mid(DATE_LASTUSED_ADDR_START, ADDRESS_LENGTH);
}
