#ifndef MESSAGEPROTOCOLMINI_H
#define MESSAGEPROTOCOLMINI_H

#include "IMessageProtocol.h"

class MessageProtocolMini : public IMessageProtocol
{
public:
    MessageProtocolMini();

    // IMessageProtocol interface
    virtual QVector<QByteArray> createPackets(const QByteArray &data, MPCmd::Command c) override;
    virtual Common::MPStatus getStatus(const QByteArray &data) override;
    virtual quint16 getMessageSize(const QByteArray &data) override;
    virtual MPCmd::Command getCommand(const QByteArray &data) override;

    virtual quint8 getFirstPayloadByte(const QByteArray &data) override;
    virtual quint8 getPayloadByteAt(const QByteArray &data, int at) override;
    virtual QByteArray getFullPayload(const QByteArray &data) override;
    virtual QByteArray getPayloadBytes(const QByteArray &data, int fromPayload, int to) override;

    virtual quint32 getSerialNumber(const QByteArray &data) override;
    virtual QVector<QByteArray> createWriteNodePackets(const QByteArray& data, const QByteArray& address) override;
    //This default func only checks if return value from device is ok or not
    virtual AsyncFuncDone getDefaultFuncDone() override;
};

#endif // MESSAGEPROTOCOLMINI_H
