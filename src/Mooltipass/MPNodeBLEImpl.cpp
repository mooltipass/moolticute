#include "MPNodeBLEImpl.h"

MPNodeBLEImpl::MPNodeBLEImpl()
{

}

bool MPNodeBLEImpl::isValid(QByteArray data)
{
    return (data.size() == 264 || data.size() == 528);
}
