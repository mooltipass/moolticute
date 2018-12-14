#include "MessageProtocolMini.h"

MessageProtocolMini::MessageProtocolMini()
{

}

void MessageProtocolMini::getPackets(/*OUT*/ QByteArray &packet, const QByteArray &data, MPCmd::Command c)
{
    packet.append(static_cast<char>(data.size()));
    packet.append(static_cast<char>(c));
    packet.append(data);
}

Common::MPStatus MessageProtocolMini::getStatus(const QByteArray &data)
{
    return Common::MPStatus(data[MP_PAYLOAD_FIELD_INDEX]);
}

quint8 MessageProtocolMini::getMessageSize(const QByteArray &data)
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
