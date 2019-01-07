#include "MessageProtocolMini.h"

MessageProtocolMini::MessageProtocolMini()
{
}

QVector<QByteArray> MessageProtocolMini::createPackets(const QByteArray &data, MPCmd::Command c)
{
    QByteArray packet;
    packet.append(static_cast<char>(data.size()));
    packet.append(static_cast<char>(c));
    packet.append(data);
    return {packet};
}

Common::MPStatus MessageProtocolMini::getStatus(const QByteArray &data)
{
    return Common::MPStatus(data[MP_PAYLOAD_FIELD_INDEX]);
}

quint16 MessageProtocolMini::getMessageSize(const QByteArray &data)
{
    return static_cast<quint8>(data[MP_LEN_FIELD_INDEX]);
}

MPCmd::Command MessageProtocolMini::getCommand(const QByteArray &data)
{
    return MPCmd::Command(data[MP_CMD_FIELD_INDEX]);
}

quint8 MessageProtocolMini::getFirstPayloadByte(const QByteArray &data)
{
    return static_cast<quint8>(data[MP_PAYLOAD_FIELD_INDEX]);
}

quint8 MessageProtocolMini::getPayloadByteAt(const QByteArray &data, int at)
{
    return static_cast<quint8>(data[MP_PAYLOAD_FIELD_INDEX + at]);
}

QByteArray MessageProtocolMini::getFullPayload(const QByteArray &data)
{
    return data.mid(MP_PAYLOAD_FIELD_INDEX, getMessageSize(data));
}

QByteArray MessageProtocolMini::getPayloadBytes(const QByteArray &data, int fromPayload, int to)
{
    return data.mid(MP_PAYLOAD_FIELD_INDEX + fromPayload, to);
}

quint32 MessageProtocolMini::getSerialNumber(const QByteArray &data)
{
    return static_cast<quint8>(data[MP_PAYLOAD_FIELD_INDEX+3]) +
            static_cast<quint32>(static_cast<quint8>(data[MP_PAYLOAD_FIELD_INDEX+2]) << 8) +
            static_cast<quint32>(static_cast<quint8>(data[MP_PAYLOAD_FIELD_INDEX+1]) << 16) +
            static_cast<quint32>(static_cast<quint8>(data[MP_PAYLOAD_FIELD_INDEX+0]) << 24);
}

QVector<QByteArray> MessageProtocolMini::createWriteNodePackets(const QByteArray &data, const QByteArray &address)
{
    QVector<QByteArray> createdPackets;
    for (quint8 i = 0; i < 3; i++)
    {
        quint8 payload_size = MP_MAX_PACKET_LENGTH - MP_PAYLOAD_FIELD_INDEX;
        if (i == 2)
        {
            payload_size = 17;
        }

        QByteArray packetToSend = QByteArray();
        packetToSend.append(address);
        packetToSend.append(i);
        packetToSend.append(data.mid(i*59, payload_size-3));
        createdPackets.append(packetToSend);
    }
    return createdPackets;
}

AsyncFuncDone MessageProtocolMini::getDefaultFuncDone()
{
    return [](const QByteArray &data, bool &) -> bool
    {
        return static_cast<quint8>(data[MP_PAYLOAD_FIELD_INDEX]) == 0x01;
    };
}
