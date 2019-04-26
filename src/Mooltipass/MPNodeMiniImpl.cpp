#include "MPNodeMiniImpl.h"
#include "Common.h"
#include "MooltipassCmds.h"

MPNodeMiniImpl::MPNodeMiniImpl(const QByteArray &d)
    : MPNodeImpl(d)
{

}

bool MPNodeMiniImpl::isValid()
{
    return data.size() == MP_NODE_SIZE &&
            (static_cast<quint8>(data[1]) & 0x20) == 0;
}

bool MPNodeMiniImpl::isDataLengthValid()
{
    return data.size() == MP_NODE_SIZE;
}

QByteArray MPNodeMiniImpl::getService() const
{
    return data.mid(8, MP_NODE_SIZE - 8 - 3);
}

void MPNodeMiniImpl::setService(QByteArray service)
{
    data.replace(SERVICE_ADDR_START, MP_MAX_PAYLOAD_LENGTH, service);
}

QByteArray MPNodeMiniImpl::getStartDataCtr() const
{
    return data.mid(CTR_DATA_ADDR_START, CTR_DATA_LENGTH);
}

QByteArray MPNodeMiniImpl::getCTR() const
{
    return data.mid(CTR_ADDR_START, CTR_DATA_LENGTH);
}

QByteArray MPNodeMiniImpl::getDescription() const
{
    return data.mid(DESC_ADDR_START, DESC_LENGTH);
}

void MPNodeMiniImpl::setDescription(const QByteArray &desc)
{
    data.replace(DESC_ADDR_START, MP_MAX_DESC_LENGTH, desc);
}

QByteArray MPNodeMiniImpl::getLogin() const
{
    return data.mid(LOGIN_ADDR_START, LOGIN_LENGTH);
}

void MPNodeMiniImpl::setLogin(const QByteArray &login)
{
    data.replace(LOGIN_ADDR_START, MP_MAX_PAYLOAD_LENGTH, login);
}

QByteArray MPNodeMiniImpl::getPasswordEnc() const
{
    return data.mid(PWD_ENC_ADDR_START, PWD_ENC_LENGTH);
}

QByteArray MPNodeMiniImpl::getDateCreated() const
{
    return data.mid(DATE_CREATED_ADDR_START, ADDRESS_LENGTH);
}

QByteArray MPNodeMiniImpl::getDateLastUsed() const
{
    return data.mid(DATE_LASTUSED_ADDR_START, ADDRESS_LENGTH);
}
