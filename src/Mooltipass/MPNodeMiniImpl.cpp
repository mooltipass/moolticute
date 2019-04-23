#include "MPNodeMiniImpl.h"
#include "Common.h"

MPNodeMiniImpl::MPNodeMiniImpl()
{

}

bool MPNodeMiniImpl::isValid(QByteArray data)
{
    return data.size() == MP_NODE_SIZE &&
               ((quint8)data[1] & 0x20) == 0;
}
