#include "DeviceDetector.h"

DeviceDetector::DeviceDetector(QObject *parent) : QObject(parent)
{

}

void DeviceDetector::setDeviceType(Common::MPHwVersion devType)
{
    if (m_deviceType != devType)
    {
        qDebug() << "Device type changed from: " << m_deviceType << " to: " << devType;
        m_deviceType = devType;
        emit deviceChanged(devType);
    }
}

bool DeviceDetector::isBle() const
{
    return Common::MP_BLE == m_deviceType;
}

bool DeviceDetector::isMini() const
{
    return Common::MP_Mini == m_deviceType;
}

bool DeviceDetector::isAdvancedMode() const
{
    return isBle() && m_advancedMode;
}

void DeviceDetector::onAdvancedModeChanged(bool isEnabled)
{
    m_advancedMode = isEnabled;
}
