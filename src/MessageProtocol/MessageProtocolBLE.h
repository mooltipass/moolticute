#ifndef MESSAGEPROTOCOLBLE_H
#define MESSAGEPROTOCOLBLE_H

#include "IMessageProtocol.h"

class MessageProtocolBLE : public IMessageProtocol
{
public:
    MessageProtocolBLE();

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
    virtual QString getDeviceName() override;

    virtual QByteArray toByteArray(const QString &input) override;

    void resetFlipBit();
    inline void setAckFlag(bool on);

private:
    inline void flipBit();
    virtual void fillCommandMapping() override;
    int getStartingPayloadPosition(const QByteArray &data) const;

    quint8 m_ackFlag = 0x00;
    quint8 m_flipBit = 0x00;

    static constexpr quint8 MESSAGE_FLIP_BIT = 0x80;
    static constexpr quint8 ACK_FLAG_BIT = 0x40;
    static constexpr int HID_PACKET_DATA_PAYLOAD = 62;
    static constexpr quint8 CMD_LOWER_BYTE = 2;
    static constexpr quint8 CMD_UPPER_BYTE = 3;
    static constexpr quint8 PAYLOAD_LEN_LOWER_BYTE = 4;
    static constexpr quint8 PAYLOAD_LEN_UPPER_BYTE = 5;
    static constexpr quint8 FIRST_PAYLOAD_BYTE_MESSAGE = 6;
    static constexpr quint8 FIRST_PAYLOAD_BYTE_PACKET = 2;
};

#endif // MESSAGEPROTOCOLBLE_H
