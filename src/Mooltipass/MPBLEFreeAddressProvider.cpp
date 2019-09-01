#include "MPBLEFreeAddressProvider.h"
#include "MessageProtocolBLE.h"

MPBLEFreeAddressProvider::MPBLEFreeAddressProvider(MessageProtocolBLE *mesProt, MPDevice *dev):
    bleProt(mesProt),
    mpDev(dev)
{

}

QByteArray MPBLEFreeAddressProvider::getFreeAddress(const int virtualAddr)
{
    if (m_parentAddrMapping.contains(virtualAddr))
    {
        return m_parentAddrMapping[virtualAddr];
    }
    else if (m_childAddrMapping.contains(virtualAddr))
    {
        return m_childAddrMapping[virtualAddr];
    }
    qCritical() << "No address loaded for virtual address: " << virtualAddr;
    return QByteArray{};
}

void MPBLEFreeAddressProvider::loadFreeAddresses(AsyncJobs *jobs, const QByteArray &addressFrom, const MPDeviceProgressCb &cbProgress)
{
    if (0 == m_parentNodeNeeded && 0 == m_childNodeNeeded)
    {
        qDebug() << "No free address is required";
        return;
    }

    FreeAddressInfo addressInfo;
    addressInfo.addressPackage = addressFrom;
    auto freeAddrNum = MAX_FREE_ADDR_REQ;
    addressInfo.parentNodeRequested = getNodeAskedNumber(MPNode::NodeParent, freeAddrNum);
    addressInfo.childNodeRequested = getNodeAskedNumber(MPNode::NodeChild, freeAddrNum);
    addressInfo.addressPackage.append(bleProt->toLittleEndianFromInt(addressInfo.parentNodeRequested));
    addressInfo.addressPackage.append(bleProt->toLittleEndianFromInt(addressInfo.childNodeRequested));

    addressInfo.startingPosition = 0;
    jobs->append(createGetFreeAddressPackage(jobs, cbProgress, addressInfo));
}

void MPBLEFreeAddressProvider::cleanFreeAddresses()
{
    m_parentNodeNeeded = 0;
    m_parentAddrMapping.clear();
    m_childNodeNeeded = 0;
    m_childAddrMapping.clear();
}

quint16 MPBLEFreeAddressProvider::getNodeAskedNumber(MPNode::NodeType nodeType, int &freeAddrNum)
{
    auto& m_nodeNeeded = MPNode::NodeParent == nodeType ? m_parentNodeNeeded : m_childNodeNeeded;
    if (freeAddrNum)
    {
        if (m_nodeNeeded > freeAddrNum)
        {
            int nodeNeededNum = freeAddrNum;
            m_nodeNeeded -= freeAddrNum;
            freeAddrNum = 0;
            return nodeNeededNum;
        }
        else
        {
            int nodeNeededNum = m_nodeNeeded;
            m_nodeNeeded = 0;
            freeAddrNum -= nodeNeededNum;
            return nodeNeededNum;
        }
    }
    return 0;
}

void MPBLEFreeAddressProvider::processReceivedAddrNumber(MPNode::NodeType nodeType, const QByteArray &receivedAddr, int &pos)
{
    auto& m_addrMapping = MPNode::NodeParent == nodeType ? m_parentAddrMapping : m_childAddrMapping;
    for (auto& ba : m_addrMapping)
    {
        if (ba.isEmpty())
        {
            ba.append(receivedAddr.mid(pos, MPNode::ADDRESS_LENGTH));
            pos += MPNode::ADDRESS_LENGTH;
        }
    }
}

void MPBLEFreeAddressProvider::loadRemainingFreeAddresses(AsyncJobs *jobs, const QByteArray &addressFrom, const MPDeviceProgressCb &cbProgress, bool isLastChild)
{
    auto freeAddrNum = MAX_FREE_ADDR_REQ - 1;
    FreeAddressInfo addressInfo;
    addressInfo.addressPackage = addressFrom;
    addressInfo.parentNodeRequested = getNodeAskedNumber(MPNode::NodeParent, freeAddrNum);
    addressInfo.childNodeRequested = getNodeAskedNumber(MPNode::NodeChild, freeAddrNum);
    if (isLastChild)
    {
        ++addressInfo.childNodeRequested;
    }
    else
    {
        ++addressInfo.parentNodeRequested;
    }
    addressInfo.addressPackage.append(bleProt->toLittleEndianFromInt(addressInfo.parentNodeRequested));
    addressInfo.addressPackage.append(bleProt->toLittleEndianFromInt(addressInfo.childNodeRequested));
    addressInfo.startingPosition = MPNode::ADDRESS_LENGTH;

    jobs->prepend(createGetFreeAddressPackage(jobs, cbProgress, addressInfo));
}

MPCommandJob* MPBLEFreeAddressProvider::createGetFreeAddressPackage(AsyncJobs *jobs, const MPDeviceProgressCb &cbProgress, MPBLEFreeAddressProvider::FreeAddressInfo addressInfo)
{
    return new MPCommandJob(mpDev, MPCmd::GET_FREE_ADDRESSES,
                                      addressInfo.addressPackage,
                                      [this, addressInfo, jobs, cbProgress](const QByteArray &data, bool &) -> bool
            {
                const auto msgSize = bleProt->getMessageSize(data);
                const bool correctMsgSize = (addressInfo.parentNodeRequested + addressInfo.childNodeRequested) * MPNode::ADDRESS_LENGTH == msgSize;
                if (1 == msgSize || !correctMsgSize)
                {
                    qCritical() << "Not enough address retrieved during loadFreeAddresses";
                    return false;
                }

                const auto receivedAddresses = bleProt->getFullPayload(data);
                qDebug() << receivedAddresses.toHex();

                int pos = addressInfo.startingPosition;

                processReceivedAddrNumber(MPNode::NodeParent, receivedAddresses, pos);
                processReceivedAddrNumber(MPNode::NodeChild, receivedAddresses, pos);

                if (m_parentNodeNeeded + m_childNodeNeeded > 0)
                {
                    qDebug() << "Need more addresses: " << m_parentNodeNeeded + m_childNodeNeeded;
                    const auto lastAddr = receivedAddresses.right(2);
                    qDebug() << "Last addr: " << lastAddr.toHex();
                    loadRemainingFreeAddresses(jobs, lastAddr, cbProgress, addressInfo.childNodeRequested > 0);
                }

                return true;
            }
        );
}
