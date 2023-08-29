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

    auto addressPackage = addressFrom;
    auto freeAddrNum = MAX_FREE_ADDR_REQ;
    FreeAddressInfo addressInfo;
    addressInfo.parentNodeRequested = getNodeAskedNumber(MPNode::NodeParent, freeAddrNum);
    addressInfo.childNodeRequested = getNodeAskedNumber(MPNode::NodeChild, freeAddrNum);
    addressPackage.append(bleProt->toLittleEndianFromInt(addressInfo.parentNodeRequested));
    addressPackage.append(bleProt->toLittleEndianFromInt(addressInfo.childNodeRequested));

    addressInfo.startingPosition = 0;
    jobs->append(createGetFreeAddressPackage(jobs, cbProgress, addressInfo, addressPackage));
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
    /*
     * During loading the remaining free addresses, the first one
     * will be the same as addressFrom, so the useful free address
     * which we are receiving is decreased by one
     */
    auto freeAddrNum = MAX_FREE_ADDR_REQ - 1;
    auto addressPackage = addressFrom;
    FreeAddressInfo addressInfo;
    addressInfo.parentNodeRequested = getNodeAskedNumber(MPNode::NodeParent, freeAddrNum);
    addressInfo.childNodeRequested = getNodeAskedNumber(MPNode::NodeChild, freeAddrNum);
    /*
     * Increasing the requested node number of addressFrom's type,
     * because it will be received again in the response.
     */
    if (isLastChild)
    {
        ++addressInfo.childNodeRequested;
    }
    else
    {
        ++addressInfo.parentNodeRequested;
    }
    addressPackage.append(bleProt->toLittleEndianFromInt(addressInfo.parentNodeRequested));
    addressPackage.append(bleProt->toLittleEndianFromInt(addressInfo.childNodeRequested));
    /*
     *  Skipping the first received address,
     *  because it is the same as addressFrom
     */
    addressInfo.startingPosition = MPNode::ADDRESS_LENGTH;

    jobs->prepend(createGetFreeAddressPackage(jobs, cbProgress, addressInfo, addressPackage));
}

MPCommandJob* MPBLEFreeAddressProvider::createGetFreeAddressPackage(AsyncJobs *jobs, const MPDeviceProgressCb &cbProgress, MPBLEFreeAddressProvider::FreeAddressInfo addressInfo, const QByteArray& addrPackage)
{
    return new MPCommandJob(mpDev, MPCmd::GET_FREE_ADDRESSES,
                                      addrPackage,
                                      [this, addressInfo, jobs, cbProgress](const QByteArray &data, bool &) -> bool
            {
                const auto msgSize = bleProt->getMessageSize(data);
                const bool correctMsgSize = (addressInfo.parentNodeRequested + addressInfo.childNodeRequested) * MPNode::ADDRESS_LENGTH == msgSize;
                if (1 == msgSize || !correctMsgSize)
                {
                    qCritical() << "Not enough address retrieved during loadFreeAddresses";
                    jobs->failCurrent();
                    return false;
                }

                const auto receivedAddresses = bleProt->getFullPayload(data);
                qDebug() << receivedAddresses.toHex();

                int pos = addressInfo.startingPosition;

                processReceivedAddrNumber(MPNode::NodeParent, receivedAddresses, pos);
                processReceivedAddrNumber(MPNode::NodeChild, receivedAddresses, pos);

                // There are more needed nodes
                if (m_parentNodeNeeded + m_childNodeNeeded > 0)
                {
                    qDebug() << "Need more addresses: " << m_parentNodeNeeded + m_childNodeNeeded;
                    /*
                     * Retreiving the last address, which will be the
                     * starting search address of the next GET_FREE_ADDRESSES
                     */
                    const auto lastAddr = receivedAddresses.right(MPNode::ADDRESS_LENGTH);
                    qDebug() << "Last addr: " << lastAddr.toHex();
                    /*
                     * If we havent received child node in the currect package
                     * last address belongs to a parent node
                     */
                    loadRemainingFreeAddresses(jobs, lastAddr, cbProgress, addressInfo.childNodeRequested > 0);
                }

                return true;
            }
        );
}
