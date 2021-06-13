#ifndef MPMINITOBLENODECONVERTER_H
#define MPMINITOBLENODECONVERTER_H

#include <QByteArray>


class MPMiniToBleNodeConverter
{
public:
    MPMiniToBleNodeConverter();
    void convert(QByteArray &dataArray, bool isData);
    QByteArray convertDataChildNode(const QByteArray& packet, int totalSize, int& current, QByteArray& addr);
private:
    QByteArray convertMiniParentNodeToBle(const QByteArray& dataArray, bool isData);
    QByteArray convertMiniChildNodeToBle(const QByteArray& dataArray);

    const static char ZERO_BYTE = static_cast<char>(0x00);
    const static quint16 CHILD_NODE_FLAG = 0x40;
    const static quint16 PARENT_ADDRESSES = 8;
    const static quint16 CHILD_ADDRESSES = 6;
    const static quint16 MINI_SERVICE_LAST_BYTE = 128;
    const static quint16 CTR_VALUE_SIZE = 3;
    const static quint16 ASCII_FLAG = 4;
    const static quint16 FLAGS_NOT_VALID_BIT = 5;
    const static quint16 DATES_SIZE = 4;
    const static quint16 KEY_PRESSED_SIZE = 4;
    const static quint16 MINI_DATES_START_BYTE = 30;
    const static quint16 MINI_LOGIN_START_BYTE = 37;
    const static quint16 MINI_LOGIN_LAST_BYTE = 100;
    const static quint16 MINI_DESC_LAST_BYTE = 30;
    const static quint16 MINI_CTR_START_BYTE = 34;
    const static quint16 MINI_PWD_START_BYTE = 100;
    const static quint16 MINI_PWD_SIZE = 32;
    const static quint16 FLAGS_BYTE_NOT_VALID_SET = 264;
    const static quint16 THIRD_FIELD_SIZE = 72;
    const static quint16 DATA_FLAGS_AND_NEXT_ADDR = 4;
    const static quint16 PACKET_ENCRYPTED_SIZE = 512;
    const static quint16 DATA_NEXT_ADDR_STARTING = 2;
};

#endif // MPMINITOBLENODECONVERTER_H
