#include "DeviceConnectionChecker.h"
#include "AppGui.h"

const QString DeviceConnectionChecker::CONNECTED_DEVICES_GROUP = "ConnectedDevices/";
const QString DeviceConnectionChecker::SERIAL_NUMBER_STR = "serial_number";

DeviceConnectionChecker::DeviceConnectionChecker(QObject *parent)
    : QObject{parent}
{
    m_connectedDevicesIni = AppGui::getDataDirPath() + "/connectedDevices.ini";
}

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
