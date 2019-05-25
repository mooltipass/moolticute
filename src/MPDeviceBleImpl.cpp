#include "MPDeviceBleImpl.h"
#include "AsyncJobs.h"
#include "MessageProtocolBLE.h"
#include "MPSettings.h"

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

    jobs->append(new MPCommandJob(mpDev, MPCmd::GET_PLAT_INFO, bleProt->getDefaultSizeCheckFuncDone()));

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
        mpDev->settings()->set_flashMbSize(memorySize);
    });

    dequeueAndRun(jobs);
}

void MPDeviceBleImpl::getDebugPlatInfo(const MessageHandlerCbData &cb)
{
    auto *jobs = new AsyncJobs("Get Debug PlatInfo", mpDev);

    jobs->append(new MPCommandJob(mpDev, MPCmd::CMD_DBG_GET_PLAT_INFO, bleProt->getDefaultSizeCheckFuncDone()));

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
    /**
     * Stoping statusTimer to avoid sending
     * MOOLTIPASS_STATUS message during flash.
     */
    mpDev->statusTimer->stop();
    if (type != "aux" && type != "main")
    {
        qCritical() << "Flashing called for an invalid type of " << type;
        cb(false, "Failed flash.");
        return;
    }
    auto *jobs = new AsyncJobs(QString("Flashing %1 MCU").arg(type), this);
    const bool isAuxFlash = type == "aux";
    const quint8 cmd = isAuxFlash ? MPCmd::CMD_DBG_FLASH_AUX_MCU : MPCmd::CMD_DBG_REBOOT_TO_BOOTLOADER;
    auto flashJob = new MPCommandJob(mpDev, cmd, bleProt->getDefaultFuncDone());
    flashJob->setReturnCheck(false);
    jobs->append(flashJob);
    if (isAuxFlash)
    {
        QSettings s;
        s.setValue(AFTER_AUX_FLASH_SETTING, true);
    }

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
        const auto flashTimeout = 5000;
        waitingJob->append(new TimerJob{flashTimeout});
        /**
          * Restart statusTimer after Main MCU flash
          */
        QTimer::singleShot(flashTimeout, [this]() {mpDev->statusTimer->start();});
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
                            {"msg", "Erasing device data..." }
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

void MPDeviceBleImpl::fetchData(QString filePath, MPCmd::Command cmd)
{
    fetchState = Common::FetchState::STARTED;
    auto *jobs = new AsyncJobs(QString("Fetch Data"), this);

    jobs->append(new MPCommandJob(mpDev, static_cast<quint8>(cmd),
                    [this, filePath, cmd](const QByteArray &data, bool &) -> bool
                    {
                        QFile *file = new QFile(filePath);
                        if (file->open(QIODevice::WriteOnly))
                        {
                            file->write(bleProt->getFullPayload(data));
                        }
                        writeFetchData(file, cmd);
                        return true;
                    }));

    dequeueAndRun(jobs);
}

void MPDeviceBleImpl::storeCredential(const BleCredential &cred, MessageHandlerCb cb)
{
    auto *jobs = new AsyncJobs(QString("Store Credential"), this);

    jobs->append(new MPCommandJob(mpDev, MPCmd::CHECK_CREDENTIAL, createCheckCredMessage(cred),
                        [this, cb, jobs, cred] (const QByteArray &data, bool &)
                        {
                            if (MSG_SUCCESS != bleProt->getFirstPayloadByte(data))
                            {
                                jobs->prepend(new MPCommandJob(mpDev, MPCmd::STORE_CREDENTIAL, createStoreCredMessage(cred),
                                   [this, cb](const QByteArray &data, bool &)
                                   {
                                       if (MSG_SUCCESS == bleProt->getFirstPayloadByte(data))
                                       {
                                           qDebug() << "Credential stored successfully";
                                           cb(true, "");
                                       }
                                       else
                                       {
                                           qWarning() << "Credential store failed";
                                           cb(false, "Credential store failed");
                                       }
                                       return true;
                                   }));
                            }
                            else
                            {
                                qWarning() << "Credential is already exists";
                                cb(false, "Credential is already exists");
                            }
                            return true;
                        }
               ));

    dequeueAndRun(jobs);
}

void MPDeviceBleImpl::getCredential(const QString& service, const QString& login, const QString& reqid, const MessageHandlerCbData &cb)
{
    AsyncJobs *jobs;
    const QString getCred = "Get Credential";
    if (reqid.isEmpty())
    {
        jobs = new AsyncJobs(getCred, this);
    }
    else
    {
        jobs = new AsyncJobs(getCred, reqid, this);
    }

    jobs->append(new MPCommandJob(mpDev, MPCmd::GET_CREDENTIAL, createGetCredMessage(service, login),
                            [this, service, login, cb](const QByteArray &data, bool &)
                            {
                                if (MSG_FAILED == bleProt->getMessageSize(data))
                                {
                                    qWarning() << "Credential get failed";
                                    cb(false, "Get credential failed", QByteArray{});
                                    return true;
                                }
                                qDebug() << "Credential got successfully";
#ifdef DEV_DEBUG
                                qDebug() << data.toHex();
#endif
                                cb(true, "", bleProt->getFullPayload(data));
                                return true;
                            }));

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        qCritical() << "Failed getting credential: " << failedJob->getErrorStr();
        cb(false, failedJob->getErrorStr(), QByteArray{});
    });

    dequeueAndRun(jobs);
}

BleCredential MPDeviceBleImpl::retrieveCredentialFromResponse(QByteArray response, QString service, QString login) const
{
    BleCredential cred{service, login};
    const int MSG_ATTR_STARTING_INDEX = 8;
    const int ATTR_NUM = 4;
    int lastIndex = MSG_ATTR_STARTING_INDEX;
    BleCredential::CredAttr attr = BleCredential::CredAttr::LOGIN;
    for (int attrIndex = 2, i = 0; i < ATTR_NUM; attrIndex+=2, ++i)
    {
        auto index = 0;
        if (BleCredential::CredAttr::PASSWORD != attr)
        {
            auto indexByte = response.mid(attrIndex, 2);
            index = bleProt->toIntFromLittleEndian(static_cast<quint8>(indexByte[0]), static_cast<quint8>(indexByte[1])) * 2 + MSG_ATTR_STARTING_INDEX;
        }
        else
        {
            //Password is between its index and the end of the message
            index = response.size();
        }
        QString attrVal = bleProt->toQString(response.mid(lastIndex, index - lastIndex - 2));

        if (cred.get(attr).isEmpty())
        {
            cred.set(attr, attrVal);
        }
#ifdef DEV_DEBUG
        qDebug() << "nextIndex: " << index << " attrName: " << attrVal;
#endif
        lastIndex = index;
        attr = static_cast<BleCredential::CredAttr>(static_cast<int>(attr) + 1);
    }

    return cred;
}

void MPDeviceBleImpl::flipBit()
{
    m_flipBit = m_flipBit ? 0x00 : MESSAGE_FLIP_BIT;
}

void MPDeviceBleImpl::resetFlipBit()
{
    m_flipBit = 0x00;
}

void MPDeviceBleImpl::sendResetFlipBit()
{
    QByteArray clearFlip;
    clearFlip.fill(static_cast<char>(0xFF), 2);
    mpDev->platformWrite(clearFlip);
    resetFlipBit();
}

void MPDeviceBleImpl::setCurrentFlipBit(QByteArray &msg)
{
    msg[0] = static_cast<char>((msg[0]&MESSAGE_ACK_AND_PAYLOAD_LENGTH)|m_flipBit);
}

void MPDeviceBleImpl::flipMessageBit(QVector<QByteArray>& msgVec)
{
    for (auto& msg : msgVec)
    {
        setCurrentFlipBit(msg);
    }
    flipBit();
}

void MPDeviceBleImpl::flipMessageBit(QByteArray &msg)
{
    setCurrentFlipBit(msg);
    flipBit();
}

bool MPDeviceBleImpl::isAfterAuxFlash()
{
    QSettings s;
    if (s.value(AFTER_AUX_FLASH_SETTING, false).toBool())
    {
        s.setValue(AFTER_AUX_FLASH_SETTING, false);
        return true;
    }
    return false;
}

void MPDeviceBleImpl::getUserCategories(const MessageHandlerCbData &cb)
{
    AsyncJobs *jobs = new AsyncJobs("Get User Categories", this);

    jobs->append(new MPCommandJob(mpDev, MPCmd::GET_USER_CATEGORIES,
                            [this, cb](const QByteArray &data, bool &)
                            {
                                if (MSG_SUCCESS == bleProt->getMessageSize(data))
                                {
                                    qWarning() << "Get user categories failed";
                                    cb(false, "Get user categories failed", QByteArray{});
                                    return true;
                                }
                                qDebug() << "User categories got successfully";

                                cb(true, "", bleProt->getFullPayload(data));
                                return true;
                            }));

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        qCritical() << "Failed getting user categories: " << failedJob->getErrorStr();
        cb(false, failedJob->getErrorStr(), QByteArray{});
    });

    dequeueAndRun(jobs);
}

void MPDeviceBleImpl::setUserCategories(const QJsonObject &categories, const MessageHandlerCbData &cb)
{
    AsyncJobs *jobs = new AsyncJobs("Set User Categories", this);

    jobs->append(new MPCommandJob(mpDev, MPCmd::SET_USER_CATEGORIES,
                                  createUserCategoriesMsg(categories),
                            [this, cb](const QByteArray &data, bool &)
                            {
                                if (MSG_FAILED == bleProt->getFirstPayloadByte(data))
                                {
                                    qWarning() << "Set user categories failed";
                                    cb(false, "Set user categories failed", QByteArray{});
                                    return true;
                                }
                                qDebug() << "User categories set successfully";
                                cb(true, "", QByteArray{});
                                return true;
                            }));

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        qCritical() << "Failed setting user categories: " << failedJob->getErrorStr();
        cb(false, failedJob->getErrorStr(), QByteArray{});
    });

    dequeueAndRun(jobs);
}

void MPDeviceBleImpl::fillGetCategory(const QByteArray& data, QJsonObject &categories)
{
    for (int i = 0; i < USER_CATEGORY_COUNT; ++i)
    {
        QString catName = "category_" + QString::number(i+1);
        QString category = bleProt->toQString(data.mid(i*USER_CATEGORY_LENGTH, (i+1)*USER_CATEGORY_LENGTH));
        bool defaultCat = std::all_of(std::begin(category), std::end(category),
                                   [](const QChar& c) { return c == QChar{0xFFFF};});
        categories[catName] = defaultCat ? "" : category;
    }
}

QByteArray MPDeviceBleImpl::createUserCategoriesMsg(const QJsonObject &categories)
{
    QByteArray data;
    for (int i = 0; i < USER_CATEGORY_COUNT; ++i)
    {
        QByteArray categoryArr = bleProt->toByteArray(categories["category_" + QString::number(i+1)].toString());
        categoryArr.append(USER_CATEGORY_LENGTH - categoryArr.size(), static_cast<char>(0x00));
        data.append(categoryArr);
    }
    return data;
}

void MPDeviceBleImpl::readUserSettings(const QByteArray& settings)
{
    quint8 d = settings[0];
    if (d != m_currentUserSettings)
    {
        set_loginPrompt(d&LOGIN_PROMPT);
        set_PINforMMM(d&PIN_FOR_MMM);
        set_storagePrompt(d&STORAGE_PROMPT);
        set_advancedMenu(d&ADVANCED_MENU);
        set_bluetoothEnabled(d&BLUETOOTH_ENABLED);
        set_credentialDisplayPrompt(d&CREDENTIAL_PROMPT);
        m_currentUserSettings = d;
        sendUserSettings();
    }
}

void MPDeviceBleImpl::sendUserSettings()
{
    QJsonObject settingJson;
    settingJson["login_prompt"] = get_loginPrompt();
    settingJson["pin_for_mmm"] = get_PINforMMM();
    settingJson["storage_prompt"] = get_storagePrompt();
    settingJson["advanced_menu"] = get_advancedMenu();
    settingJson["bluetooth_enabled"] = get_bluetoothEnabled();
    settingJson["credential_prompt"] = get_credentialDisplayPrompt();
    emit userSettingsChanged(settingJson);
}

QByteArray MPDeviceBleImpl::createStoreCredMessage(const BleCredential &cred)
{
    return createCredentialMessage(cred.getAttributes());
}

QByteArray MPDeviceBleImpl::createGetCredMessage(QString service, QString login)
{
    CredMap getMsgMap{{BleCredential::CredAttr::SERVICE, service},
                      {BleCredential::CredAttr::LOGIN, login}};
    return createCredentialMessage(getMsgMap);
}

QByteArray MPDeviceBleImpl::createCheckCredMessage(const BleCredential &cred)
{
    auto checkCred = cred.getAttributes();
    checkCred.remove(BleCredential::CredAttr::DESCRIPTION);
    checkCred.remove(BleCredential::CredAttr::THIRD);
    return createCredentialMessage(checkCred);
}

QByteArray MPDeviceBleImpl::createCredentialMessage(const CredMap &credMap)
{
    QByteArray storeMessage;
    quint16 index = static_cast<quint16>(0);
    QByteArray credDatas;
    for (QString attr: credMap)
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
    if (MSG_SUCCESS == bleProt->getFirstPayloadByte(data))
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
                                          {"msg", "Writing bundle data to device..." }
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

void MPDeviceBleImpl::writeFetchData(QFile *file, MPCmd::Command cmd)
{
    mpDev->sendData(cmd,
                    [this, file, cmd](bool, const QByteArray &data, bool &) -> bool
                    {
                        file->write(bleProt->getFullPayload(data));
                        if (Common::FetchState::STARTED == fetchState)
                        {
                            writeFetchData(file, cmd);
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
