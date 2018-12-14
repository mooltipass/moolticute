#ifndef MESSAGEPROTOCOLMINI_H
#define MESSAGEPROTOCOLMINI_H

#include "IMessageProtocol.h"

class MessageProtocolMini : public IMessageProtocol
{
public:
    MessageProtocolMini();

    // IMessageProtocol interface
    virtual void getPackets(QByteArray &packet, const QByteArray &data, MPCmd::Command c) override;
    virtual Common::MPStatus getStatus(const QByteArray &data) override;
    virtual quint8 getMessageSize(const QByteArray &data) override;
    virtual MPCmd::Command getCommand(const QByteArray &data) override;
    virtual quint8 getFirstPayloadByte(const QByteArray &data) override;
};

#endif // MESSAGEPROTOCOLMINI_H
