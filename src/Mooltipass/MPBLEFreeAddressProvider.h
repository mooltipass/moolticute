#ifndef MPBLEFREEADDRESSPROVIDER_H
#define MPBLEFREEADDRESSPROVIDER_H

#include "MPNode.h"
#include "MPDevice.h"

class MessageProtocolBLE;
class MPBLEFreeAddressProvider
{
    struct FreeAddressInfo
    {
        int parentNodeRequested = 0;
        int childNodeRequested = 0;
        int startingPosition = 0;
    };

public:
    MPBLEFreeAddressProvider(MessageProtocolBLE *mesProt, MPDevice *dev);

    void incrementParentNodeNeeded(int mappingAddr) { ++m_parentNodeNeeded; m_parentAddrMapping[mappingAddr] = QByteArray{}; }
    void incrementChildNodeNeeded(int mappingAddr) { ++m_childNodeNeeded; m_childAddrMapping[mappingAddr] = QByteArray{}; }
    QByteArray getFreeAddress(const int virtualAddr);
    void loadFreeAddresses(AsyncJobs *jobs, const QByteArray &addressFrom, const MPDeviceProgressCb &cbProgress);
    void cleanFreeAddresses();
    void addTestParentAddress(const int virtualAddr, QByteArray addr) { m_parentAddrMapping[virtualAddr] = addr; }

private:
    /**
     * @brief getNodeAskedNumber
     * @param nodeType: The type of requested address (child/parent)
     * @param freeAddrNum: Remaining free address number for the next packet
     * @return Number of addresses which will requested in the next packet of nodeType
     */
    quint16 getNodeAskedNumber(MPNode::NodeType nodeType, int& freeAddrNum);
    void processReceivedAddrNumber(MPNode::NodeType nodeType, const QByteArray& receivedAddr, int& pos);
    void loadRemainingFreeAddresses(AsyncJobs *jobs, const QByteArray &addressFrom, const MPDeviceProgressCb &cbProgress, bool isLastChild);
    MPCommandJob* createGetFreeAddressPackage(AsyncJobs *jobs, const MPDeviceProgressCb &cbProgress, FreeAddressInfo addressInfo, const QByteArray& addrPackage);


    MessageProtocolBLE *bleProt;
    MPDevice *mpDev;

    int m_parentNodeNeeded = 0;
    int m_childNodeNeeded = 0;
    QMap<int, QByteArray> m_parentAddrMapping;
    QMap<int, QByteArray> m_childAddrMapping;

    static constexpr int MAX_FREE_ADDR_REQ = 266;
};

#endif // MPBLEFREEADDRESSPROVIDER_H
