#ifndef IMESSAGEPROTOCOL_H
#define IMESSAGEPROTOCOL_H

#include "../MooltipassCmds.h"
#include "../Common.h"

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
};


#endif // IMESSAGEPROTOCOL_H
