#ifndef IMESSAGEPROTOCOL_H
#define IMESSAGEPROTOCOL_H

#include "../MooltipassCmds.h"
#include "../Common.h"
#include "../AsyncJobs.h"

#include <QByteArray>

class IMessageProtocol
{
public:
    IMessageProtocol() = default;
    virtual ~IMessageProtocol() = default;

    virtual void getPackets(/*OUT*/ QByteArray&, const QByteArray &, MPCmd::Command) = 0;
    virtual Common::MPStatus getStatus(const QByteArray &) = 0;
    virtual quint8 getMessageSize(const QByteArray &) = 0;
    virtual MPCmd::Command getCommand(const QByteArray &) = 0;
    virtual quint8 getFirstPayloadByte(const QByteArray &) = 0;
    virtual QByteArray getFullPayload(const QByteArray &) = 0;
    virtual quint8 getPayloadByteAt(const QByteArray &data, int at) = 0;
    virtual QByteArray getPayloadBytes(const QByteArray &, int fromPayload, int to) = 0;
    virtual quint32 getSerialNumber(const QByteArray &) = 0;
    virtual QVector<QByteArray> createWriteNodePackets(const QByteArray& data, const QByteArray& address) = 0;
    //This default func only checks if return value from device is ok or not
    virtual AsyncFuncDone getDefaultFuncDone() = 0;
};


#endif // IMESSAGEPROTOCOL_H
