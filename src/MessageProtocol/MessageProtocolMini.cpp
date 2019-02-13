#include "MessageProtocolMini.h"

MessageProtocolMini::MessageProtocolMini()
{
    fillCommandMapping();
}

QVector<QByteArray> MessageProtocolMini::createPackets(const QByteArray &data, MPCmd::Command c)
{
    QByteArray packet;
    packet.append(static_cast<char>(data.size()));
    const auto miniCommandIter = m_commandMapping.find(c);
    if (miniCommandIter == m_commandMapping.end())
    {
        qCritical() << MPCmd::printCmd(c) << " is not implemented for Mini";
        packet.append(static_cast<char>(m_commandMapping[MPCmd::PING]));
        return {packet};
    }
    const quint16 commandId = miniCommandIter.value();
    packet.append(static_cast<char>(commandId));
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
    return MPCmd::Command(m_commandMapping.key(static_cast<quint8>(data[MP_CMD_FIELD_INDEX])));
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

QString MessageProtocolMini::getDeviceName()
{
    return "Mini";
}

QByteArray MessageProtocolMini::toByteArray(const QString &input)
{
    return input.toUtf8();
}

QString MessageProtocolMini::toQString(const QByteArray &data)
{
    return QString::fromUtf8(data);
}

void MessageProtocolMini::fillCommandMapping()
{
    m_commandMapping = {
        {MPCmd::EXPORT_FLASH_START    , 0x8A},
        {MPCmd::EXPORT_FLASH          , 0x8B},
        {MPCmd::EXPORT_FLASH_END      , 0x8C},
        {MPCmd::IMPORT_FLASH_BEGIN    , 0x8D},
        {MPCmd::IMPORT_FLASH          , 0x8E},
        {MPCmd::IMPORT_FLASH_END      , 0x8F},
        {MPCmd::EXPORT_EEPROM_START   , 0x90},
        {MPCmd::EXPORT_EEPROM         , 0x91},
        {MPCmd::EXPORT_EEPROM_END     , 0x92},
        {MPCmd::IMPORT_EEPROM_BEGIN   , 0x93},
        {MPCmd::IMPORT_EEPROM         , 0x94},
        {MPCmd::IMPORT_EEPROM_END     , 0x95},
        {MPCmd::ERASE_EEPROM          , 0x96},
        {MPCmd::ERASE_FLASH           , 0x97},
        {MPCmd::ERASE_SMC             , 0x98},
        {MPCmd::DRAW_BITMAP           , 0x99},
        {MPCmd::SET_FONT              , 0x9A},
        {MPCmd::USB_KEYBOARD_PRESS    , 0x9B},
        {MPCmd::STACK_FREE            , 0x9C},
        {MPCmd::CLONE_SMARTCARD       , 0x9D},
        {MPCmd::DEBUG                 , 0xA0},
        {MPCmd::PING                  , 0xA1},
        {MPCmd::VERSION               , 0xA2},
        {MPCmd::CONTEXT               , 0xA3},
        {MPCmd::GET_LOGIN             , 0xA4},
        {MPCmd::GET_PASSWORD          , 0xA5},
        {MPCmd::SET_LOGIN             , 0xA6},
        {MPCmd::SET_PASSWORD          , 0xA7},
        {MPCmd::CHECK_PASSWORD        , 0xA8},
        {MPCmd::ADD_CONTEXT           , 0xA9},
        {MPCmd::SET_BOOTLOADER_PWD    , 0xAA},
        {MPCmd::JUMP_TO_BOOTLOADER    , 0xAB},
        {MPCmd::GET_RANDOM_NUMBER     , 0xAC},
        {MPCmd::START_MEMORYMGMT      , 0xAD},
        {MPCmd::IMPORT_MEDIA_START    , 0xAE},
        {MPCmd::IMPORT_MEDIA          , 0xAF},
        {MPCmd::IMPORT_MEDIA_END      , 0xB0},
        {MPCmd::SET_MOOLTIPASS_PARM   , 0xB1},
        {MPCmd::GET_MOOLTIPASS_PARM   , 0xB2},
        {MPCmd::RESET_CARD            , 0xB3},
        {MPCmd::READ_CARD_LOGIN       , 0xB4},
        {MPCmd::READ_CARD_PASS        , 0xB5},
        {MPCmd::SET_CARD_LOGIN        , 0xB6},
        {MPCmd::SET_CARD_PASS         , 0xB7},
        {MPCmd::ADD_UNKNOWN_CARD      , 0xB8},
        {MPCmd::MOOLTIPASS_STATUS     , 0xB9},
        {MPCmd::FUNCTIONAL_TEST_RES   , 0xBA},
        {MPCmd::SET_DATE              , 0xBB},
        {MPCmd::SET_UID               , 0xBC},
        {MPCmd::GET_UID               , 0xBD},
        {MPCmd::SET_DATA_SERVICE      , 0xBE},
        {MPCmd::ADD_DATA_SERVICE      , 0xBF},
        {MPCmd::WRITE_32B_IN_DN       , 0xC0},
        {MPCmd::READ_32B_IN_DN        , 0xC1},
        {MPCmd::GET_CUR_CARD_CPZ      , 0xC2},
        {MPCmd::CANCEL_USER_REQUEST   , 0xC3},
        {MPCmd::PLEASE_RETRY          , 0xC4},
        {MPCmd::READ_FLASH_NODE       , 0xC5},
        {MPCmd::WRITE_FLASH_NODE      , 0xC6},
        {MPCmd::GET_FAVORITE          , 0xC7},
        {MPCmd::SET_FAVORITE          , 0xC8},
        {MPCmd::GET_STARTING_PARENT   , 0xC9},
        {MPCmd::SET_STARTING_PARENT   , 0xCA},
        {MPCmd::GET_CTRVALUE          , 0xCB},
        {MPCmd::SET_CTRVALUE          , 0xCC},
        {MPCmd::ADD_CARD_CPZ_CTR      , 0xCD},
        {MPCmd::GET_CARD_CPZ_CTR      , 0xCE},
        {MPCmd::CARD_CPZ_CTR_PACKET   , 0xCF},
        {MPCmd::GET_30_FREE_SLOTS     , 0xD0},
        {MPCmd::GET_DN_START_PARENT   , 0xD1},
        {MPCmd::SET_DN_START_PARENT   , 0xD2},
        {MPCmd::END_MEMORYMGMT        , 0xD3},
        {MPCmd::SET_USER_CHANGE_NB    , 0xD4},
        {MPCmd::GET_DESCRIPTION       , 0xD5},
        {MPCmd::GET_USER_CHANGE_NB    , 0xD6},
        {MPCmd::SET_DESCRIPTION       , 0xD8},
        {MPCmd::LOCK_DEVICE           , 0xD9},
        {MPCmd::GET_SERIAL            , 0xDA},
    };
}
