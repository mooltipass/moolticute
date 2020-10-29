#include "MPMiniToBleNodeConverter.h"
#include "Common.h"

MPMiniToBleNodeConverter::MPMiniToBleNodeConverter()
{

}

void MPMiniToBleNodeConverter::convert(QByteArray &dataArray)
{
    const bool childNode = dataArray[1]&CHILD_NODE_FLAG;

    if (childNode)
    {
        dataArray = convertMiniChildNodeToBle(dataArray);
    }
    else
    {
        dataArray = convertMiniParentNodeToBle(dataArray);
    }
}

QByteArray MPMiniToBleNodeConverter::convertMiniParentNodeToBle(const QByteArray &dataArray)
{
    // Flags, prev, next parent, first child
    QByteArray bleArray = dataArray.left(PARENT_ADDRESSES);
    // Converting service name to ascii
    for (int i = PARENT_ADDRESSES; i <= MINI_SERVICE_LAST_BYTE; ++i)
    {
        bleArray.append(dataArray[i]);
        bleArray.append(ZERO_BYTE);
    }
    // Fill remaining service name
    const quint16 REMAINING_SERVICE_NAME = 10;
    bleArray.append(REMAINING_SERVICE_NAME, ZERO_BYTE);
    // Reserved
    bleArray.append(ZERO_BYTE);
    // CTR value
    bleArray.append(dataArray.right(CTR_VALUE_SIZE));
    return bleArray;
}

QByteArray MPMiniToBleNodeConverter::convertMiniChildNodeToBle(const QByteArray &dataArray)
{
    // Flags, prev, next parent, first child
    QByteArray bleArray = dataArray.left(CHILD_ADDRESSES);
    bleArray[0] = dataArray[0]|(1<<ASCII_FLAG); // Setting ascii flag for child node
    // Pointed to child
    Common::fill(bleArray, 2, ZERO_BYTE);
    // Last modified/used day
    bleArray.append(dataArray.mid(MINI_DATES_START_BYTE, DATES_SIZE));
    // Convert login to ascii
    for (int i = MINI_LOGIN_START_BYTE; i < MINI_LOGIN_LAST_BYTE; ++i)
    {
        bleArray.append(dataArray[i]);
        bleArray.append(ZERO_BYTE);
    }
    // Fill remaining login
    Common::fill(bleArray, 2, ZERO_BYTE);
    // Convert description to ascii
    for (int i = CHILD_ADDRESSES; i < MINI_DESC_LAST_BYTE; ++i)
    {
        bleArray.append(dataArray[i]);
        bleArray.append(ZERO_BYTE);
    }
    // Fill arbitrary third field
    bleArray.append(THIRD_FIELD_SIZE, ZERO_BYTE);
    // Key pressed
    const char DEFAULT_CHAR = static_cast<char>(0xFF);
    bleArray.append(KEY_PRESSED_SIZE, DEFAULT_CHAR);
    // same as flags, but with bit 5 set to 1
    bleArray.append(bleArray.left(2));
    bleArray[FLAGS_BYTE_NOT_VALID_SET] = bleArray[FLAGS_BYTE_NOT_VALID_SET]|(1<<FLAGS_NOT_VALID_BIT);
    // reserved
    bleArray.append(ZERO_BYTE);
    // CTR value
    bleArray.append(dataArray.mid(MINI_CTR_START_BYTE, CTR_VALUE_SIZE));
    // Encrypted password
    bleArray.append(dataArray.mid(MINI_PWD_START_BYTE, MINI_PWD_SIZE));
    const quint16 REMAINING_PASSWORD_SIZE = 96;
    bleArray.append(REMAINING_PASSWORD_SIZE, ZERO_BYTE);
    // password terminating 0
    Common::fill(bleArray, 2, ZERO_BYTE);
    // TBD
    const quint16 TBD_SIZE = 128;
    bleArray.append(TBD_SIZE, ZERO_BYTE);

    return bleArray;
}
