#ifndef MPDEVICEBLEIMPL_H
#define MPDEVICEBLEIMPL_H

#include "MPDevice.h"
#include "BleCommon.h"

class MessageProtocolBLE;

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
    void fetchData(QString filePath, MPCmd::Command cmd);
    inline void stopFetchData() { fetchState = Common::FetchState::STOPPED; }

    void storeCredential(const BleCredential &cred, MessageHandlerCb cb);
    void getCredential(const QString& service, const QString& login, const QString& reqid, const MessageHandlerCbData &cb);
    BleCredential retrieveCredentialFromResponse(QByteArray response, QString service, QString login) const;

    void sendResetFlipBit();
    void setCurrentFlipBit(QByteArray &msg);
    void flipMessageBit(QVector<QByteArray> &msg);
    void flipMessageBit(QByteArray &msg);

    void getUserCategories(const MessageHandlerCbData &cb);
    void fillGetCategory(const QByteArray& data, QJsonObject &categories);

private:
    void checkDataFlash(const QByteArray &data, QElapsedTimer *timer, AsyncJobs *jobs, QString filePath, const MPDeviceProgressCb &cbProgress);
    void sendBundleToDevice(QString filePath, AsyncJobs *jobs, const MPDeviceProgressCb &cbProgress);
    void writeFetchData(QFile *file, MPCmd::Command cmd);

    QByteArray createStoreCredMessage(const BleCredential &cred);
    QByteArray createGetCredMessage(QString service, QString login);
    QByteArray createCheckCredMessage(const BleCredential &cred);
    QByteArray createCredentialMessage(const CredMap &credMap);

    void dequeueAndRun(AsyncJobs *job);

    inline void flipBit();
    void resetFlipBit();

    MessageProtocolBLE *bleProt;
    MPDevice *mpDev;
    Common::FetchState fetchState = Common::FetchState::STOPPED;

    quint8 m_flipBit = 0x00;

    static constexpr quint8 MESSAGE_FLIP_BIT = 0x80;
    static constexpr quint8 MESSAGE_ACK_AND_PAYLOAD_LENGTH = 0x7F;
    static constexpr int BUNBLE_DATA_WRITE_SIZE = 256;
    static constexpr int BUNBLE_DATA_ADDRESS_SIZE = 4;
    static constexpr int USER_CATEGORY_COUNT = 4;
    static constexpr int USER_CATEGORY_LENGTH = 66;
};

#endif // MPDEVICEBLEIMPL_H
