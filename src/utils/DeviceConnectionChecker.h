#ifndef DEVICECONNECTIONCHECKER_H
#define DEVICECONNECTIONCHECKER_H

#include <QObject>
#include "Common.h"
#include "KeyboardLayoutDetector.h"

class DeviceConnectionChecker : public QObject
{
    Q_OBJECT
    DISABLE_COPY_MOVE(DeviceConnectionChecker)
private:
    explicit DeviceConnectionChecker(QObject *parent = nullptr);

    DeviceConnectionChecker *_instance = nullptr;
public:
    static DeviceConnectionChecker & instance()
    {
        static DeviceConnectionChecker * _instance = nullptr;
        if (nullptr == _instance)
        {
            _instance = new DeviceConnectionChecker{};
        }
        return *_instance;
    }
    ~DeviceConnectionChecker()
    {
        delete _instance;
    }

public:
    void registerDevice(const QString& serialNum);
    bool wasConnected(const QString& serialNum);
    bool isNewDevice(quint32 serialNum);

signals:
    void newDeviceDetected();

private:
    QString m_connectedDevicesIni;
    KeyboardLayoutDetector m_layoutDetector{};

    static const QString CONNECTED_DEVICES_FILE;
    static const QString CONNECTED_DEVICES_GROUP;
    static const QString SERIAL_NUMBER_STR;

};

#endif // DEVICECONNECTIONCHECKER_H
