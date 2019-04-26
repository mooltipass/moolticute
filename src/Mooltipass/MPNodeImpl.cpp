#include "MPNodeImpl.h"

#include "Common.h"

MPNodeImpl::MPNodeImpl(const QByteArray &d)
    : data(d)
{
}

int MPNodeImpl::getType() const
{
    if (data.size() > 1)
    {
        return (static_cast<quint8>(data[1]) >> 6) & 0x03;
    }
    return -1;
}

void MPNodeImpl::setType(quint8 type)
{
    if (data.size() > 1)
    {
        data[1] = type << 6;
    }
}

QByteArray MPNodeImpl::getData() const
{
    return data;
}

void MPNodeImpl::appendData(const QByteArray &d)
{
    data.append(d);
}

QByteArray MPNodeImpl::getNodeFlags()
{
    return data.mid(NODE_FLAG_ADDR_START, ADDRESS_LENGTH);
}

QByteArray MPNodeImpl::getPreviousAddress() const
{
    return data.mid(PREVIOUS_PARENT_ADDR_START, ADDRESS_LENGTH);
}

void MPNodeImpl::setPreviousAddress(const QByteArray &d)
{
    data[PREVIOUS_PARENT_ADDR_START] = d[0];
    data[PREVIOUS_PARENT_ADDR_START+1] = d[1];
}

QByteArray MPNodeImpl::getNextAddress() const
{
    return data.mid(NEXT_PARENT_ADDR_START, ADDRESS_LENGTH);
}

void MPNodeImpl::setNextAddress(const QByteArray &d)
{
    data[NEXT_PARENT_ADDR_START] = d[0];
    data[NEXT_PARENT_ADDR_START+1] = d[1];
}

QByteArray MPNodeImpl::getStartAddress() const
{
    return data.mid(START_CHILD_ADDR_START, ADDRESS_LENGTH);
}

void MPNodeImpl::setStartAddress(const QByteArray &d)
{
    data[START_CHILD_ADDR_START] = d[0];
    data[START_CHILD_ADDR_START+1] = d[1];
}

QByteArray MPNodeImpl::getNextChildDataAddress() const
{
    return data.mid(NEXT_DATA_ADDR_START, ADDRESS_LENGTH);
}

void MPNodeImpl::setNextChildDataAddress(const QByteArray &d)
{
    data[NEXT_DATA_ADDR_START] = d[0];
    data[NEXT_DATA_ADDR_START+1] = d[1];
}

QByteArray MPNodeImpl::getNextDataAddress() const
{
    return data.mid(NEXT_DATA_ADDR_START, ADDRESS_LENGTH);
}

void MPNodeImpl::setLoginNodeData(const QByteArray &flags, const QByteArray &d)
{
    data.replace(DATA_ADDR_START, MP_NODE_SIZE-8, d);
    data.replace(0, 2, flags);
}

QByteArray MPNodeImpl::getLoginNodeData() const
{
    return data.mid(DATA_ADDR_START);
}

void MPNodeImpl::setLoginChildNodeData(const QByteArray &flags, const QByteArray &d)
{
    data.replace(LOGIN_CHILD_NODE_DATA_ADDR_START, MP_NODE_SIZE-8, d);
    data.replace(0, ADDRESS_LENGTH, flags);
}

QByteArray MPNodeImpl::getLoginChildNodeData() const
{
    return data.mid(LOGIN_CHILD_NODE_DATA_ADDR_START);
}

void MPNodeImpl::setDataNodeData(const QByteArray &flags, const QByteArray &d)
{
    data.replace(DATA_ADDR_START, MP_NODE_SIZE-8, d);
    data.replace(0, ADDRESS_LENGTH, flags);
}

QByteArray MPNodeImpl::getDataNodeData() const
{
    return data.mid(DATA_ADDR_START);
}

void MPNodeImpl::setDataChildNodeData(const QByteArray &flags, const QByteArray &d)
{
    data.replace(DATA_CHILD_DATA_ADDR_START, MP_NODE_SIZE-4, d);
    data.replace(0, ADDRESS_LENGTH, flags);
}

QByteArray MPNodeImpl::getDataChildNodeData() const
{
    return data.mid(DATA_CHILD_DATA_ADDR_START);
}
