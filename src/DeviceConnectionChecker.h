#ifndef DEVICECONNECTIONCHECKER_H
#define DEVICECONNECTIONCHECKER_H

#include <QObject>

class DeviceConnectionChecker : public QObject
{
    Q_OBJECT
public:
    explicit DeviceConnectionChecker(QObject *parent = nullptr);
    void registerDevice(const QString& serialNum);
    bool wasConnected(const QString& serialNum);
    bool isNewDevice(quint32 serialNum);

private:
    QString m_connectedDevicesIni;
    static const QString CONNECTED_DEVICES_GROUP;
    static const QString SERIAL_NUMBER_STR;

};

#endif // DEVICECONNECTIONCHECKER_H
