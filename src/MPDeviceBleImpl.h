#ifndef MPDEVICEBLEIMPL_H
#define MPDEVICEBLEIMPL_H

#include "MPDevice.h"
#include <QMap>

class MessageProtocolBLE;

class Credential
{
public:
    enum class CredValue
    {
        SERVICE,
        LOGIN,
        PASSWORD,
        DESCRIPTION,
        THIRD
    };

    Credential(QString service, QString login = "", QString pwd = "", QString desc = "", QString third = "")
    {
        m_attributes = { {CredValue::SERVICE, service},
                         {CredValue::LOGIN, login},
                         {CredValue::PASSWORD, pwd},
                         {CredValue::DESCRIPTION, desc},
                         {CredValue::THIRD, third}
                       };
    }

    QMap<CredValue, QString> getAttributes() const { return m_attributes; }
    void set(CredValue field, QString value) { m_attributes[field] = value; }
    QString get(CredValue field) const { return m_attributes[field]; }

private:
    QMap<CredValue, QString> m_attributes;
};

/**
 * @brief The MPDeviceBleImpl class
 * Implementations of only BLE related commands
 * and helper functions
 */
class MPDeviceBleImpl: public QObject
{
    Q_OBJECT

    QT_WRITABLE_PROPERTY(QString, mainMCUVersion, QString())
    QT_WRITABLE_PROPERTY(QString, auxMCUVersion, QString())

public:
    MPDeviceBleImpl(MessageProtocolBLE *mesProt, MPDevice *dev);

    bool isFirstPacket(const QByteArray &data);
    bool isLastPacket(const QByteArray &data);

    void getPlatInfo();
    void getDebugPlatInfo(const MessageHandlerCbData &cb);
    QVector<int> calcDebugPlatInfo(const QByteArray &platInfo);

    void flashMCU(QString type, const MessageHandlerCb &cb);
    void uploadBundle(QString filePath, const MessageHandlerCb &cb, const MPDeviceProgressCb &cbProgress);
    void fetchAccData(QString filePath);
    inline void stopFetchAccData() { accState = Common::AccState::STOPPED; }

    void sendResetFlipBit();

    QByteArray getStoreMessage(Credential cred);

private:
    void checkDataFlash(const QByteArray &data, QElapsedTimer *timer, AsyncJobs *jobs, QString filePath, const MPDeviceProgressCb &cbProgress);
    void sendBundleToDevice(QString filePath, AsyncJobs *jobs, const MPDeviceProgressCb &cbProgress);
    void writeAccData(QFile *file);

    void dequeueAndRun(AsyncJobs *job);


    MessageProtocolBLE *bleProt;
    MPDevice *mpDev;
    Common::AccState accState = Common::AccState::STOPPED;

    static constexpr int BUNBLE_DATA_WRITE_SIZE = 256;
    static constexpr int BUNBLE_DATA_ADDRESS_SIZE = 4;
};

#endif // MPDEVICEBLEIMPL_H
