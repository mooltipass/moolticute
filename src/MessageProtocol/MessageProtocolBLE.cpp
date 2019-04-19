#include "MessageProtocolBLE.h"

MessageProtocolBLE::MessageProtocolBLE()
{
    fillCommandMapping();
}

QVector<QByteArray> MessageProtocolBLE::createPackets(const QByteArray &data, MPCmd::Command c)
{
    QByteArray messagePayload;
    const auto bleCommandIter = m_commandMapping.find(c);
    if (bleCommandIter == m_commandMapping.end())
    {
        qCritical() << MPCmd::printCmd(c) << " is not implemented for BLE";
        return QVector<QByteArray>();
    }
    const quint16 bleCommandId = bleCommandIter.value();
    messagePayload.append(static_cast<char>(bleCommandId&0xFF));
    messagePayload.append(static_cast<char>((bleCommandId&0xFF00)>>8));
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
    return Common::Unlocked;
}

quint16 MessageProtocolBLE::getMessageSize(const QByteArray &data)
{
    return (static_cast<quint8>(data[PAYLOAD_LEN_LOWER_BYTE])|static_cast<quint16>((data[PAYLOAD_LEN_UPPER_BYTE]<<8)));
}

MPCmd::Command MessageProtocolBLE::getCommand(const QByteArray &data)
{
   const quint16 bleCommandId = toIntFromLittleEndian(static_cast<quint8>(data[CMD_LOWER_BYTE]), static_cast<quint8>(data[CMD_UPPER_BYTE]));
   return MPCmd::Command(m_commandMapping.key(bleCommandId));
}

quint8 MessageProtocolBLE::getFirstPayloadByte(const QByteArray &data)
{
    return static_cast<quint8>(data[FIRST_PAYLOAD_BYTE_MESSAGE]);
}

quint8 MessageProtocolBLE::getPayloadByteAt(const QByteArray &data, int at)
{
    return static_cast<quint8>(data[at + getStartingPayloadPosition(data)]);
}

QByteArray MessageProtocolBLE::getFullPayload(const QByteArray &data)
{
    int startingPos = getStartingPayloadPosition(data);
    if (FIRST_PAYLOAD_BYTE_PACKET == startingPos)
    {
        return data.mid(startingPos);
    }
    return data.mid(startingPos, getMessageSize(data));
}

QByteArray MessageProtocolBLE::getPayloadBytes(const QByteArray &data, int fromPayload, int to)
{
    int start = getStartingPayloadPosition(data);
    return data.mid(fromPayload + start, (to - fromPayload) + 1);
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
        return true;
    };
}

QString MessageProtocolBLE::getDeviceName()
{
    return "BLE";
}

QByteArray MessageProtocolBLE::toByteArray(const QString &input)
{
    //Convert string to unicode byte array (2 bytes for 1 char)
    QByteArray unicodeArray;
    for (QChar ch : input)
    {
        quint16 uniChar = ch.unicode();
        unicodeArray.append(static_cast<char>((0xFF&uniChar)));
        unicodeArray.append(static_cast<char>((0xFF00&uniChar)>>8));
    }
    return unicodeArray;
}

QString MessageProtocolBLE::toQString(const QByteArray &data)
{
    QString out = "";
    const auto size = data.size();
    for (int i = 0; i < size; i+=2)
    {
        if (i+1 >= size)
        {
            qCritical() << "Out of bounds";
            break;
        }
        quint16 unicode = static_cast<quint8>(data[i]);
        unicode |= (data[i+1]<<8);
        out += QChar(unicode);
    }
    return out;
}

void MessageProtocolBLE::setAckFlag(bool on)
{
    m_ackFlag = on ? ACK_FLAG_BIT : 0x00;
}

void MessageProtocolBLE::flipMessageBit(QByteArray &msg)
{
    flipBit();
    msg[0] = msg[0]^MESSAGE_FLIP_BIT;
}

void MessageProtocolBLE::resetFlipBit()
{
    m_flipBit = 0x00;
}

void MessageProtocolBLE::flipBit()
{
    m_flipBit = m_flipBit ? 0x00 : MESSAGE_FLIP_BIT;
}

void MessageProtocolBLE::fillCommandMapping()
{
    //TODO fill commandId mapping, when they are implemented for BLE
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
        {MPCmd::PING                  , 0x0001},
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
        {MPCmd::GET_RANDOM_NUMBER     , 0x0008},
        {MPCmd::START_MEMORYMGMT      , 0x0009},
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
        {MPCmd::MOOLTIPASS_STATUS     , 0x0001},
        {MPCmd::FUNCTIONAL_TEST_RES   , 0xBA},
        {MPCmd::SET_DATE              , 0x0004},
        {MPCmd::SET_UID               , 0xBC},
        {MPCmd::GET_UID               , 0xBD},
        {MPCmd::SET_DATA_SERVICE      , 0xBE},
        {MPCmd::ADD_DATA_SERVICE      , 0xBF},
        {MPCmd::WRITE_32B_IN_DN       , 0xC0},
        {MPCmd::READ_32B_IN_DN        , 0xC1},
        {MPCmd::GET_CUR_CARD_CPZ      , 0xC2},
        {MPCmd::CANCEL_USER_REQUEST   , 0xC3},
        {MPCmd::PLEASE_RETRY          , 0x0002},
        {MPCmd::READ_FLASH_NODE       , 0x0102},
        {MPCmd::WRITE_FLASH_NODE      , 0xC6},
        {MPCmd::GET_FAVORITE          , 0xC7},
        {MPCmd::SET_FAVORITE          , 0xC8},
        {MPCmd::GET_STARTING_PARENT   , 0x0100},
        {MPCmd::SET_STARTING_PARENT   , 0xCA},
        {MPCmd::GET_CTRVALUE          , 0xCB},
        {MPCmd::SET_CTRVALUE          , 0xCC},
        {MPCmd::ADD_CARD_CPZ_CTR      , 0xCD},
        {MPCmd::GET_CARD_CPZ_CTR      , 0xCE},
        {MPCmd::CARD_CPZ_CTR_PACKET   , 0xCF},
        {MPCmd::GET_30_FREE_SLOTS     , 0xD0},
        {MPCmd::GET_DN_START_PARENT   , 0xD1},
        {MPCmd::SET_DN_START_PARENT   , 0xD2},
        {MPCmd::END_MEMORYMGMT        , 0x0101},
        {MPCmd::SET_USER_CHANGE_NB    , 0xD4},
        {MPCmd::GET_DESCRIPTION       , 0xD5},
        {MPCmd::GET_USER_CHANGE_NB    , 0xD6},
        {MPCmd::SET_DESCRIPTION       , 0xD8},
        {MPCmd::LOCK_DEVICE           , 0xD9},
        {MPCmd::GET_SERIAL            , 0xDA},
        {MPCmd::CMD_DBG_MESSAGE       , 0x8000},
        {MPCmd::GET_PLAT_INFO         , 0x0003},
        {MPCmd::STORE_CREDENTIAL      , 0x0006},
        {MPCmd::GET_CREDENTIAL        , 0x0007},
        {MPCmd::CMD_DBG_OPEN_DISP_BUFFER    , 0x8001},
        {MPCmd::CMD_DBG_SEND_TO_DISP_BUFFER , 0x8002},
        {MPCmd::CMD_DBG_CLOSE_DISP_BUFFER   , 0x8003},
        {MPCmd::CMD_DBG_ERASE_DATA_FLASH    , 0x8004},
        {MPCmd::CMD_DBG_IS_DATA_FLASH_READY , 0x8005},
        {MPCmd::CMD_DBG_DATAFLASH_WRITE_256B, 0x8006},
        {MPCmd::CMD_DBG_REBOOT_TO_BOOTLOADER, 0x8007},
        {MPCmd::CMD_DBG_GET_ACC_32_SAMPLES  , 0x8008},
        {MPCmd::CMD_DBG_FLASH_AUX_MCU       , 0x8009},
        {MPCmd::CMD_DBG_GET_PLAT_INFO       , 0x800A},
        {MPCmd::CMD_DBG_REINDEX_BUNDLE      , 0x800B},
    };
}

int MessageProtocolBLE::getStartingPayloadPosition(const QByteArray &data) const
{
    quint8 actPacketNum = (data[1] & 0xF0) >> 4;
    return 0 == actPacketNum ? FIRST_PAYLOAD_BYTE_MESSAGE : FIRST_PAYLOAD_BYTE_PACKET;
}
