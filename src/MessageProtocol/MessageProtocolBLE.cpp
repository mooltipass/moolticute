#include "MessageProtocolBLE.h"

MessageProtocolBLE::MessageProtocolBLE()
{

}

QVector<QByteArray> MessageProtocolBLE::createPackets(const QByteArray &data, MPCmd::Command c)
{
    QByteArray messagePayload;
    messagePayload.append(static_cast<char>(c&0xFF));
    messagePayload.append(static_cast<char>((c&0xFF00)>>8));
    const int dataSize = data.size();
    messagePayload.append(static_cast<char>(dataSize&0xFF));
    messagePayload.append(static_cast<char>((dataSize&0xFF00)>>8));
    messagePayload.append(data);
    QVector<QByteArray> packets;
    int remainingBytes = messagePayload.size();
    const int packetNum = ((remainingBytes + HID_PACKET_DATA_PAYLOAD - 1) / HID_PACKET_DATA_PAYLOAD) - 1;
    int curPacketId = 0;
    int curByteIndex = 0;
    while (curPacketId <= packetNum)
    {
        QByteArray packet;
        int payloadLength = remainingBytes < HID_PACKET_DATA_PAYLOAD ? remainingBytes : HID_PACKET_DATA_PAYLOAD;
        packet.append(static_cast<char>(m_flipBit|m_ackFlag|payloadLength));
        packet.append(static_cast<char>((curPacketId << 4)|packetNum));
        packet.append(messagePayload.mid(curByteIndex, payloadLength));

        remainingBytes -= payloadLength;
        curByteIndex += payloadLength;
        ++curPacketId;
        packets.append(packet);
    }
    flipBit();
    return packets;
}

Common::MPStatus MessageProtocolBLE::getStatus(const QByteArray &data)
{
    Q_UNUSED(data);
    qWarning("Not implemented yet");
    return Common::UnknownStatus;
}

quint16 MessageProtocolBLE::getMessageSize(const QByteArray &data)
{
    return (data[PAYLOAD_LEN_LOWER_BYTE]|(data[PAYLOAD_LEN_UPPER_BYTE]<<8));
}

MPCmd::Command MessageProtocolBLE::getCommand(const QByteArray &data)
{
   return MPCmd::Command(data[CMD_LOWER_BYTE]|(data[CMD_UPPER_BYTE]<<8));
}

quint8 MessageProtocolBLE::getFirstPayloadByte(const QByteArray &data)
{
    Q_UNUSED(data);
    qWarning("Not implemented yet");
    return 0;
}

quint8 MessageProtocolBLE::getPayloadByteAt(const QByteArray &data, int at)
{
    Q_UNUSED(data);
    Q_UNUSED(at);
    qWarning("Not implemented yet");
    return 0;
}

QByteArray MessageProtocolBLE::getFullPayload(const QByteArray &data)
{
    Q_UNUSED(data);
    qWarning("Not implemented yet");
    return QByteArray();
}

QByteArray MessageProtocolBLE::getPayloadBytes(const QByteArray &data, int fromPayload, int to)
{
    Q_UNUSED(data);
    Q_UNUSED(fromPayload);
    Q_UNUSED(to);
    qWarning("Not implemented yet");
    return QByteArray();
}

quint32 MessageProtocolBLE::getSerialNumber(const QByteArray &data)
{
    Q_UNUSED(data);
    qWarning("Not implemented yet");
    return 0;
}

QVector<QByteArray> MessageProtocolBLE::createWriteNodePackets(const QByteArray &data, const QByteArray &address)
{
    Q_UNUSED(data);
    Q_UNUSED(address);
    QVector<QByteArray> packets;
    qWarning("Not implemented yet");
    packets.append(QByteArray());
    return packets;
}

AsyncFuncDone MessageProtocolBLE::getDefaultFuncDone()
{
    return [](const QByteArray &data, bool &) -> bool
    {
        Q_UNUSED(data);
        qWarning("Not implemented yet");
        return false;
    };
}

void MessageProtocolBLE::flipBit()
{
    m_flipBit = m_flipBit ? 0x00 : MESSAGE_FLIP_BIT;
}
