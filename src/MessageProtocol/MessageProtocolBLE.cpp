#include "MessageProtocolBLE.h"

MessageProtocolBLE::MessageProtocolBLE()
{

}

QVector<QByteArray> MessageProtocolBLE::createPackets(const QByteArray &data, MPCmd::Command c)
{
    Q_UNUSED(data);
    Q_UNUSED(c);
    QVector<QByteArray> packets;
    qWarning("Not implemented yet");
    packets.append(QByteArray());
    return packets;
}

Common::MPStatus MessageProtocolBLE::getStatus(const QByteArray &data)
{
    Q_UNUSED(data);
    qWarning("Not implemented yet");
    return Common::UnknownStatus;
}

quint8 MessageProtocolBLE::getMessageSize(const QByteArray &data)
{
    Q_UNUSED(data);
    qWarning("Not implemented yet");
    return 0;
}

MPCmd::Command MessageProtocolBLE::getCommand(const QByteArray &data)
{
    Q_UNUSED(data);
    qWarning("Not implemented yet");
    return MPCmd::PING;
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
