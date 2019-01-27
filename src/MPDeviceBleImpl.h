#ifndef MPDEVICEBLEIMPL_H
#define MPDEVICEBLEIMPL_H

#include "MPDevice.h"

class MessageProtocolBLE;

class MPDeviceBleImpl: public QObject
{
    Q_OBJECT
public:
    MPDeviceBleImpl(MessageProtocolBLE *mesProt, MPDevice *dev);
    void getPlatInfo(const MessageHandlerCb &cb);
    QVector<int> calcPlatInfo();

    void flashMCU(QString type, const MessageHandlerCb &cb);
    void uploadBundle(QString filePath, const MessageHandlerCb &cb, const MPDeviceProgressCb &cbProgress);

    void sendResetFlipBit();

private:
    void checkDataFlash(const QByteArray &data, QElapsedTimer *timer, AsyncJobs *jobs, QString filePath, const MPDeviceProgressCb &cbProgress);
    void sendBundleToDevice(QString filePath, AsyncJobs *jobs, const MPDeviceProgressCb &cbProgress);

    void dequeueAndRun(AsyncJobs *job);


    MessageProtocolBLE *bleProt;
    MPDevice *mpDev;

    QByteArray platInfo;
};

#endif // MPDEVICEBLEIMPL_H
