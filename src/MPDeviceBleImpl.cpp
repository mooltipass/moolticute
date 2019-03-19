#include "MPDeviceBleImpl.h"
#include "AsyncJobs.h"
#include "MessageProtocol/MessageProtocolBLE.h"

MPDeviceBleImpl::MPDeviceBleImpl(MessageProtocolBLE* mesProt, MPDevice *dev):
    bleProt(mesProt),
    mpDev(dev)
{

}

bool MPDeviceBleImpl::isFirstPacket(const QByteArray &data)
{
    quint8 actPacket = (data[1] & 0xF0) >> 4;
    return actPacket == 0;
}

bool MPDeviceBleImpl::isLastPacket(const QByteArray &data)
{
    quint8 totalPacketNum = data[1] & 0x0F;
    quint8 actPacket = (data[1] & 0xF0) >> 4;
    return actPacket == totalPacketNum;
}

void MPDeviceBleImpl::getPlatInfo()
{
    auto *jobs = new AsyncJobs("Get PlatInfo", mpDev);

    jobs->append(new MPCommandJob(mpDev, MPCmd::GET_PLAT_INFO, bleProt->getDefaultFuncDone()));

    connect(jobs, &AsyncJobs::finished, [this](const QByteArray &data)
    {
        QByteArray response = bleProt->getFullPayload(data);
        const auto mainMajor = bleProt->toIntFromLittleEndian(static_cast<quint8>(response[0]), static_cast<quint8>(response[1]));
        const auto mainMinor = bleProt->toIntFromLittleEndian(static_cast<quint8>(response[2]), static_cast<quint8>(response[3]));
        set_mainMCUVersion(QString::number(mainMajor) + "." + QString::number(mainMinor));
        const auto auxMajor = bleProt->toIntFromLittleEndian(static_cast<quint8>(response[4]), static_cast<quint8>(response[5]));
        const auto auxMinor = bleProt->toIntFromLittleEndian(static_cast<quint8>(response[6]), static_cast<quint8>(response[7]));
        set_auxMCUVersion(QString::number(auxMajor) + "." + QString::number(auxMinor));
        const auto serialLower = bleProt->toIntFromLittleEndian(static_cast<quint8>(response[8]), static_cast<quint8>(response[9]));
        const auto serialUpper = bleProt->toIntFromLittleEndian(static_cast<quint8>(response[10]), static_cast<quint8>(response[11]));
        quint32 serialNum = serialLower;
        serialNum |= static_cast<quint32>((serialUpper<<16));
        mpDev->set_serialNumber(serialNum);
        const auto memorySize = bleProt->toIntFromLittleEndian(static_cast<quint8>(response[12]), static_cast<quint8>(response[13]));
        mpDev->set_flashMbSize(memorySize);
    });

    dequeueAndRun(jobs);
}

void MPDeviceBleImpl::getDebugPlatInfo(const MessageHandlerCbData &cb)
{
    auto *jobs = new AsyncJobs("Get Debug PlatInfo", mpDev);

    jobs->append(new MPCommandJob(mpDev, MPCmd::CMD_DBG_GET_PLAT_INFO, bleProt->getDefaultFuncDone()));

    connect(jobs, &AsyncJobs::finished, [this, cb](const QByteArray &data)
    {
        Q_UNUSED(data);
        /* Callback */
        cb(true, "", bleProt->getFullPayload(data));
    });

    dequeueAndRun(jobs);
}

QVector<int> MPDeviceBleImpl::calcDebugPlatInfo(const QByteArray &platInfo)
{
    QVector<int> platInfos;
    platInfos.append(bleProt->toIntFromLittleEndian(static_cast<quint8>(platInfo[0]), static_cast<quint8>(platInfo[1])));
    platInfos.append(bleProt->toIntFromLittleEndian(static_cast<quint8>(platInfo[2]), static_cast<quint8>(platInfo[3])));
    platInfos.append(bleProt->toIntFromLittleEndian(static_cast<quint8>(platInfo[64]), static_cast<quint8>(platInfo[65])));
    platInfos.append(bleProt->toIntFromLittleEndian(static_cast<quint8>(platInfo[66]), static_cast<quint8>(platInfo[67])));
    return platInfos;
}

void MPDeviceBleImpl::flashMCU(QString type, const MessageHandlerCb &cb)
{
    if (type != "aux" && type != "main")
    {
        qCritical() << "Flashing called for an invalid type of " << type;
        cb(false, "Failed flash.");
        return;
    }
    auto *jobs = new AsyncJobs(QString("Flashing %1 MCU").arg(type), this);
    const bool isAuxFlash = type == "aux";
    const quint8 cmd = isAuxFlash ? MPCmd::CMD_DBG_FLASH_AUX_MCU : MPCmd::CMD_DBG_REBOOT_TO_BOOTLOADER;
    jobs->append(new MPCommandJob(mpDev, cmd, bleProt->getDefaultFuncDone()));

    connect(jobs, &AsyncJobs::failed, [cb, isAuxFlash](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        if (isAuxFlash)
        {
            cb(false, "Failed to flash Aux MCU!");
        }
        else
        {
            cb(true, "");
        }
    });

    mpDev->jobsQueue.enqueue(jobs);

    if (!isAuxFlash)
    {
        /**
          * Main MCU flash does not cause a reconnect of the device,
          * so daemon keeps sending messages to the device, which
          * is causing failure in the communication, this is why
          * this sleep is added.*/

        auto *waitingJob = new AsyncJobs(QString("Waiting job for Main MCU flash"), this);
        qDebug() << "Waiting for device to start after Main MCU flash";
        waitingJob->append(new TimerJob{5000});
        mpDev->jobsQueue.enqueue(waitingJob);
    }

    mpDev->runAndDequeueJobs();
}

void MPDeviceBleImpl::uploadBundle(QString filePath, const MessageHandlerCb &cb, const MPDeviceProgressCb &cbProgress)
{
    QElapsedTimer *timer = new QElapsedTimer{};
    timer->start();
    auto *jobs = new AsyncJobs(QString("Upload bundle file"), this);
    jobs->append(new MPCommandJob(mpDev, MPCmd::CMD_DBG_ERASE_DATA_FLASH,
                      [cbProgress] (const QByteArray &, bool &) -> bool
                    {
                        QVariantMap progress = {
                            {"total", 0},
                            {"current", 0},
                            {"msg", tr("Erasing device data...") }
                        };
                        cbProgress(progress);
                        return true;
                    }));

    jobs->append(new MPCommandJob(mpDev, MPCmd::CMD_DBG_IS_DATA_FLASH_READY,
                    [this, timer, jobs, filePath, cbProgress](const QByteArray &data, bool &) -> bool
                    {
                        checkDataFlash(data, timer, jobs, filePath, cbProgress);
                        return true;
                    }));

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        cb(false, "Upload bundle failed");
    });

    connect(jobs, &AsyncJobs::finished, [cb](const QByteArray &)
    {
        cb(true, "");
    });

    dequeueAndRun(jobs);
}

void MPDeviceBleImpl::fetchAccData(QString filePath)
{
    accState = Common::AccState::STARTED;
    auto *jobs = new AsyncJobs(QString("Fetch Acc Data"), this);

    jobs->append(new MPCommandJob(mpDev, MPCmd::CMD_DBG_GET_ACC_32_SAMPLES,
                    [this, filePath](const QByteArray &data, bool &) -> bool
                    {
                        QFile *file = new QFile(filePath);
                        if (file->open(QIODevice::WriteOnly))
                        {
                            file->write(bleProt->getFullPayload(data));
                        }
                        writeAccData(file);
                        return true;
                    }));

    dequeueAndRun(jobs);
}

void MPDeviceBleImpl::sendResetFlipBit()
{
    QByteArray clearFlip;
    clearFlip.fill(static_cast<char>(0xFF), 2);
    mpDev->platformWrite(clearFlip);
    bleProt->resetFlipBit();
}

QByteArray MPDeviceBleImpl::getStoreMessage(Credential cred)
{
    QByteArray storeMessage;
    //Add Service index
    quint16 index = static_cast<quint16>(0);
    QByteArray credDatas;
    for (QString attr: cred.getAttributes())
    {
        if (attr.isEmpty())
        {
            storeMessage.append(bleProt->toLittleEndianFromInt(static_cast<quint16>(0xFFFF)));
        }
        else
        {
            storeMessage.append(bleProt->toLittleEndianFromInt(index));
            index += (attr.size() + 1);
            QByteArray data = bleProt->toByteArray(attr);
            data.append(bleProt->toLittleEndianFromInt(static_cast<quint16>(0x0)));
            credDatas.append(data);
        }
    }

    storeMessage.append(credDatas);
    return storeMessage;
}

void MPDeviceBleImpl::checkDataFlash(const QByteArray &data, QElapsedTimer *timer, AsyncJobs *jobs, QString filePath, const MPDeviceProgressCb &cbProgress)
{
    if (0x01 == bleProt->getFirstPayloadByte(data))
    {
        qDebug() << "Erase done in: " << timer->nsecsElapsed() / 1000000 << " ms";
        delete timer;
        sendBundleToDevice(filePath, jobs, cbProgress);
    }
    else
    {
        //Dataflash is not done yet.
        jobs->prepend(new MPCommandJob(mpDev, MPCmd::CMD_DBG_IS_DATA_FLASH_READY,
                       [this, timer, jobs, filePath, cbProgress](const QByteArray &data, bool &) -> bool
                       {
                           checkDataFlash(data, timer, jobs, filePath, cbProgress);
                           return true;
                       }));
    }
}

void MPDeviceBleImpl::sendBundleToDevice(QString filePath, AsyncJobs *jobs, const MPDeviceProgressCb &cbProgress)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical() << "Error opening bundle file: " << filePath;
        return;
    }
    QByteArray blob = file.readAll();
    const auto fileSize = blob.size();
    qDebug() << "Bundle size: " << fileSize;
    int byteCounter = BUNBLE_DATA_ADDRESS_SIZE;
    int curAddress = 0;
    QByteArray message;
    message.fill(static_cast<char>(0), BUNBLE_DATA_ADDRESS_SIZE);
    for (const auto byte : blob)
    {
        message.append(byte);
        ++byteCounter;
        if ((BUNBLE_DATA_WRITE_SIZE + BUNBLE_DATA_ADDRESS_SIZE) == byteCounter)
        {
            jobs->append(new MPCommandJob(mpDev, MPCmd::CMD_DBG_DATAFLASH_WRITE_256B, message,
                              [curAddress, cbProgress, fileSize](const QByteArray &data, bool &) -> bool
                                  {
                                      Q_UNUSED(data);
                                      QVariantMap progress = QVariantMap {
                                          {"total", fileSize},
                                          {"current", curAddress},
                                          {"msg", tr("Writing bundle data to device...") }
                                      };
                                      cbProgress(progress);
#ifdef DEV_DEBUG
                                      qDebug() << "Sending message to address #" << curAddress;
#endif
                                      return true;
                                  }));
            curAddress += BUNBLE_DATA_WRITE_SIZE;
            message.clear();
            quint32 qCurAddress = static_cast<quint32>(curAddress);
            //Add write address to message (Big endian)
            message.append(static_cast<char>((qCurAddress&0xFF)));
            message.append(static_cast<char>(((qCurAddress&0xFF00)>>8)));
            message.append(static_cast<char>(((qCurAddress&0xFF0000)>>16)));
            message.append(static_cast<char>(((qCurAddress&0xFF000000)>>24)));
            byteCounter = BUNBLE_DATA_ADDRESS_SIZE;
        }
    }
    jobs->append(new MPCommandJob(mpDev, MPCmd::CMD_DBG_DATAFLASH_WRITE_256B, message,
                  [](const QByteArray &data, bool &) -> bool
                      {
                          Q_UNUSED(data);
                          qDebug() << "Sending bundle is DONE";
                          return true;
                      }));

    jobs->append(new MPCommandJob(mpDev, MPCmd::CMD_DBG_REINDEX_BUNDLE, bleProt->getDefaultFuncDone()));
}

void MPDeviceBleImpl::writeAccData(QFile *file)
{
    mpDev->sendData(MPCmd::CMD_DBG_GET_ACC_32_SAMPLES,
                    [this, file](bool, const QByteArray &data, bool &) -> bool
                    {
                        file->write(bleProt->getFullPayload(data));
                        if (Common::AccState::STARTED == accState)
                        {
                            writeAccData(file);
                        }
                        else
                        {
                            delete file;
                        }
                        return true;
                    });
}

void MPDeviceBleImpl::dequeueAndRun(AsyncJobs *jobs)
{
    mpDev->jobsQueue.enqueue(jobs);
    mpDev->runAndDequeueJobs();
}
