#include "DeviceConnectionChecker.h"
#include "AppGui.h"

const QString DeviceConnectionChecker::CONNECTED_DEVICES_FILE = "connectedDevices.ini";
const QString DeviceConnectionChecker::CONNECTED_DEVICES_GROUP = "ConnectedDevices/";
const QString DeviceConnectionChecker::SERIAL_NUMBER_STR = "serial_number";

DeviceConnectionChecker::DeviceConnectionChecker(QObject *parent)
    : QObject{parent}
{
    m_connectedDevicesIni = AppGui::getDataDirPath() + "/" + CONNECTED_DEVICES_FILE;
}

/**
 * @brief DeviceConnectionChecker::registerDevice
 * Storing serial number with date of first connection
 * in connectedDevices ini file.
 * @param serialNum connected device's serial number
 */
void DeviceConnectionChecker::registerDevice(const QString &serialNum)
{
    QSettings s(m_connectedDevicesIni, QSettings::IniFormat);
    s.beginGroup(CONNECTED_DEVICES_GROUP + serialNum);

    s.setValue(SERIAL_NUMBER_STR, QDate::currentDate());

    s.endGroup();
    s.sync();
}

bool DeviceConnectionChecker::wasConnected(const QString &serialNum)
{
    QSettings s(m_connectedDevicesIni, QSettings::IniFormat);
    s.sync();
    const QString key = CONNECTED_DEVICES_GROUP + serialNum + "/" + SERIAL_NUMBER_STR;
    return s.contains(key);
}

/**
 * @brief DeviceConnectionChecker::isNewDevice
 * @param serialNum connected device's serial number
 * Checks if the connected device with given serial number
 * has been connected before, if this is the first time
 * emitting newDeviceDetected signal
 * @return true, in case of first connection of the device
 */
bool DeviceConnectionChecker::isNewDevice(quint32 serialNum)
{
    bool firstConnection = false;
    if (0 != serialNum)
    {
        QString serialStr = QString::number(serialNum);
        if (!wasConnected(serialStr))
        {
            firstConnection = true;
            DeviceConnectionChecker::instance().registerDevice(serialStr);
            emit newDeviceDetected();
        }
    }
    return firstConnection;
}
