#ifndef DEVICEDETECTOR_H
#define DEVICEDETECTOR_H

#include <QObject>
#include "Common.h"

class DeviceDetector : public QObject
{
    Q_OBJECT
private:
    explicit DeviceDetector(QObject *parent = nullptr);
    DeviceDetector(const DeviceDetector&) = delete;
    DeviceDetector(const DeviceDetector&&) = delete;
    DeviceDetector& operator=(const DeviceDetector& other) = delete;
    DeviceDetector& operator=(const DeviceDetector&& other) = delete;

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

signals:
    void deviceChanged(Common::MPHwVersion newDevType);

public slots:
    void onAdvancedModeChanged(bool isEnabled);

private:
    Common::MPHwVersion m_deviceType = Common::MP_Unknown;
    bool m_advancedMode = false;
};

#endif // DEVICEDETECTOR_H
