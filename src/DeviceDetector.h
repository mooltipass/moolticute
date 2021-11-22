#ifndef DEVICEDETECTOR_H
#define DEVICEDETECTOR_H

#include <QObject>
#include "Common.h"

class DeviceDetector : public QObject
{
    Q_OBJECT
    DISABLE_COPY_MOVE(DeviceDetector)
private:
    explicit DeviceDetector(QObject *parent = nullptr);

    DeviceDetector *_instance = nullptr;
public:
    static DeviceDetector & instance()
    {
       static DeviceDetector * _instance = nullptr;
       if (nullptr == _instance)
       {
           _instance = new DeviceDetector{};
       }
       return *_instance;
    }
    ~DeviceDetector()
    {
        delete _instance;
    }

    void setDeviceType(Common::MPHwVersion devType);
    bool isBle() const;
    bool isMini() const;

    bool isAdvancedMode() const;

    bool isConnectedWithBluetooth() const { return m_isConnectedWithBluetooth; }
    void setIsConnectedWithBluetooth(bool bt) { m_isConnectedWithBluetooth = bt; }
    quint8 getBattery() const { return m_battery; }
    void setBattery(quint8 battery) { m_battery = battery; }
    void shiftPressed() { m_isShiftPressed = true; }
    void shiftReleased() { m_isShiftPressed = false; }
    bool isShiftPressed() const { return m_isShiftPressed; }

signals:
    void deviceChanged(Common::MPHwVersion newDevType);

public slots:
    void onAdvancedModeChanged(bool isEnabled);

private:
    Common::MPHwVersion m_deviceType = Common::MP_Unknown;
    bool m_advancedMode = false;
    bool m_isConnectedWithBluetooth = false;
    quint8 m_battery = 0;
    bool m_isShiftPressed = false;
};

#endif // DEVICEDETECTOR_H
