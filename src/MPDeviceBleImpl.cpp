#include "MPDeviceBleImpl.h"
#include "AsyncJobs.h"
#include "MessageProtocolBLE.h"
#include "MPNodeBLE.h"
#include "AppDaemon.h"
#include "DeviceSettingsBLE.h"
#include "Base32.h"

int MPDeviceBleImpl::s_LangNum = 0;
int MPDeviceBleImpl::s_LayoutNum = 0;

MPDeviceBleImpl::MPDeviceBleImpl(MessageProtocolBLE* mesProt, MPDevice *dev):
    bleProt(mesProt),
    mpDev(dev),
    freeAddressProv(mesProt, dev)
{
    m_noBundleCommands = {
        MPCmd::PING,
        MPCmd::MOOLTIPASS_STATUS,
        MPCmd::START_BUNDLE_UPLOAD,
        MPCmd::WRITE_256B_TO_FLASH,
        MPCmd::END_BUNDLE_UPLOAD,
        MPCmd::CANCEL_USER_REQUEST,
        MPCmd::INFORM_LOCKED,
        MPCmd::INFORM_UNLOCKED,
        MPCmd::SET_DATE
    };
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
        const auto messageSize = bleProt->getMessageSize(data);
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
        const auto bundleVersion = bleProt->toIntFromLittleEndian(static_cast<quint8>(response[14]), static_cast<quint8>(response[15]));
        set_bundleVersion(bundleVersion);
        if (messageSize > PLATFORM_SERIAL_NUM_FIRST_BYTE)
        {
            const auto serialPlatformLower = bleProt->toIntFromLittleEndian(static_cast<quint8>(response[PLATFORM_SERIAL_NUM_FIRST_BYTE]),
                                                                            static_cast<quint8>(response[PLATFORM_SERIAL_NUM_FIRST_BYTE+1]));
            const auto serialPlatformUpper = bleProt->toIntFromLittleEndian(static_cast<quint8>(response[PLATFORM_SERIAL_NUM_FIRST_BYTE+2]),
                                                                            static_cast<quint8>(response[PLATFORM_SERIAL_NUM_FIRST_BYTE+3]));
            quint32 serialPlatformNum = serialPlatformLower;
            serialPlatformNum |= static_cast<quint32>((serialPlatformUpper<<16));
            set_platformSerialNumber(serialPlatformNum);
        }
    });

    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::enforceCategory(int category)
{
    auto *jobs = new AsyncJobs("Set current category", mpDev);

    QByteArray catArr;
    catArr.append(static_cast<char>(category));
    jobs->append(new MPCommandJob(mpDev, MPCmd::SET_CUR_CATEGORY, catArr, bleProt->getDefaultSizeCheckFuncDone()));

    mpDev->enqueueAndRunJob(jobs);
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

    mpDev->enqueueAndRunJob(jobs);
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

void MPDeviceBleImpl::flashMCU(const MessageHandlerCb &cb)
{
    /**
     * Stoping statusTimer to avoid sending
     * MOOLTIPASS_STATUS message during flash.
     */
    mpDev->statusTimer->stop();
    auto *jobs = new AsyncJobs("Flashing Aux and Main MCU", this);
    auto flashJob = new MPCommandJob(mpDev, MPCmd::CMD_DBG_UPDATE_MAIN_AUX, bleProt->getDefaultFuncDone());
    flashJob->setReturnCheck(false);
    jobs->append(flashJob);
    QSettings s;
    s.setValue(AFTER_AUX_FLASH_SETTING, true);


    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        cb(false, "Failed to flash Aux and Main MCU!");
    });

    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::uploadBundle(QString filePath, QString password, const MessageHandlerCb &cb, const MPDeviceProgressCb &cbProgress)
{
    if (!isBundleFileReadable(filePath))
    {
        qCritical() << "Error opening bundle file: " << filePath;
        cb(false, "Bundle file is not readable");
        return;
    }
    QElapsedTimer *timer = new QElapsedTimer{};
    timer->start();
    auto *jobs = new AsyncJobs(QString("Upload bundle file"), this);
    jobs->append(new MPCommandJob(mpDev, MPCmd::START_BUNDLE_UPLOAD, createUploadPasswordMessage(password),
                      [cbProgress, timer, jobs, filePath, this] (const QByteArray &data, bool &) -> bool
                    {
                        if (MSG_SUCCESS != bleProt->getFirstPayloadByte(data))
                        {
                            qWarning() << "Erase data flash failed";
                            return false;
                        }
                        QVariantMap progress = {
                            {"total", 0},
                            {"current", 0},
                            {"msg", "Erasing data finished." }
                        };
                        qDebug() << "Erase done in: " << timer->nsecsElapsed() / 1000000 << " ms";
                        delete timer;
                        sendBundleToDevice(filePath, jobs, cbProgress);
                        cbProgress(progress);
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

    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::fetchData(QString filePath, MPCmd::Command cmd)
{
    fetchState = Common::FetchState::STARTED;
    auto *jobs = new AsyncJobs(QString("Fetch Data"), this);

    jobs->append(new MPCommandJob(mpDev, static_cast<quint8>(cmd),
                    [this, filePath, cmd](const QByteArray &data, bool &) -> bool
                    {
                        QFile *file = new QFile(filePath);
                        if (file->open(DeviceOpenModeFlag::WriteOnly))
                        {
                            file->write(bleProt->getFullPayload(data));
                        }
                        writeFetchData(file, cmd);
                        return true;
                    }));

    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::fetchDataFiles()
{
    if (mpDev->get_status() != Common::Unlocked)
    {
        if (AppDaemon::isDebugDev())
        {
            qWarning() << "Device is not unlocked, cannot fetch data files";
        }
        return;
    }
    auto *jobs = new AsyncJobs(QString("Fetch data files"), this);
    m_dataFiles.clear();

    // Start fetch from 0x0000 address
    const QByteArray startingAddr(2, 0x00);
    fetchDataFiles(jobs, startingAddr);
    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::fetchDataFiles(AsyncJobs *jobs, QByteArray addr)
{
    jobs->append(new MPCommandJob(mpDev, MPCmd::FETCH_DATA_NODES, addr,
                        [this, jobs] (const QByteArray &data, bool &)
                        {
                            const int FILENAME_STARTING_POS = 2;
                            QString fileName = bleProt->toQString(bleProt->getPayloadBytes(data, FILENAME_STARTING_POS, bleProt->getMessageSize(data)));
                            if (!fileName.isEmpty())
                            {
                                m_dataFiles.append(fileName);
                            }

                            if (bleProt->getMessageSize(data) < DATA_FETCH_NO_NEXT_ADDR_SIZE)
                            {
                                qCritical() << "Invalid response size for fetch data nodes";
                                return false;
                            }

                            if (bleProt->getMessageSize(data) != DATA_FETCH_NO_NEXT_ADDR_SIZE)
                            {
                                fetchDataFiles(jobs, bleProt->getPayloadBytes(data, 0, 2));
                            }
                            else
                            {
                                emit mpDev->filesCacheChanged();
                            }
                            return true;
                        }
               ));
}

void MPDeviceBleImpl::addDataFile(const QString &file)
{
    m_dataFiles.append(file);
    emit mpDev->filesCacheChanged();
}

void MPDeviceBleImpl::deleteFile(QString file, bool isNote, std::function<void (bool)> cb)
{
    QByteArray fileNameArray = bleProt->toByteArray(file);
    fileNameArray.append(ZERO_BYTE);
    fileNameArray.append(ZERO_BYTE);
    auto *jobs = new AsyncJobs(QString("Delete File"), this);
    jobs->append(new MPCommandJob(mpDev, isNote? MPCmd::DELETE_NOTE_FILE : MPCmd::DELETE_DATA_FILE,
                                  fileNameArray,
                                  [this, file, cb, isNote](const QByteArray &data, bool &) -> bool
    {
        if (bleProt->getFirstPayloadByte(data) != MSG_SUCCESS)
        {
            qWarning() << "Remove " << file << " failed";
            cb(false);
            return false;
        }
        if (!isNote)
        {
            // For files delete removed file from cache
            m_dataFiles.removeOne(file);
        }
        cb(true);
        return true;
    }));
    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::fetchNotes()
{
    if (mpDev->get_status() != Common::Unlocked && mpDev->get_status() != Common::MMMMode)
    {
        if (AppDaemon::isDebugDev())
        {
            qWarning() << "Device is not unlocked, cannot fetch notes";
        }
        return;
    }
    if (m_fetchingNotes)
    {
        qWarning() << "Fetching notes is in progress...";
        return;
    }
    m_fetchingNotes = true;
    auto *jobs = new AsyncJobs(QString("Fetch notes"), this);
    m_notes.clear();

    // Start fetch from 0x0000 address
    const QByteArray startingAddr(2, 0x00);
    fetchNotes(jobs, startingAddr);
    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::fetchNotes(AsyncJobs *jobs, QByteArray addr)
{
    jobs->append(new MPCommandJob(mpDev, MPCmd::GET_NEXT_NOTE_ADDR, addr,
                        [this, jobs] (const QByteArray &data, bool &)
                        {
                            const int NOTENAME_STARTING_POS = 2;
                            QString noteName = bleProt->toQString(bleProt->getPayloadBytes(data, NOTENAME_STARTING_POS, bleProt->getMessageSize(data)));
                            if (!noteName.isEmpty())
                            {
                                m_notes.push_back(noteName);
                            }

                            if (bleProt->getMessageSize(data) < DATA_FETCH_NO_NEXT_ADDR_SIZE)
                            {
                                qCritical() << "Invalid response size for fetch notes";
                                m_fetchingNotes = false;
                                return false;
                            }

                            if (bleProt->getMessageSize(data) != DATA_FETCH_NO_NEXT_ADDR_SIZE)
                            {
                                fetchNotes(jobs, bleProt->getPayloadBytes(data, 0, 2));
                            }
                            else
                            {
                                m_fetchingNotes = false;
                                emit notesFetched();
                            }
                            return true;
                        }
    ));
}

void MPDeviceBleImpl::fetchNotesLater()
{
    createAndAddCustomJob("Fetch notes", [this](){fetchNotes();});
}

void MPDeviceBleImpl::getNoteNode(QString note, std::function<void (bool, QString, QString, QByteArray)> cb)
{
    if (note.isEmpty())
    {
        qWarning() << "note name is empty.";
        cb(false, "note name is empty", QString(), QByteArray());
        return;
    }


    AsyncJobs *jobs = new AsyncJobs(QString{"Getting note node"}, this);

    QByteArray noteData = bleProt->toByteArray(note);
    noteData.append(ZERO_BYTE);
    noteData.append(ZERO_BYTE);

    QVariantMap m = {{ "note", note }};
    jobs->user_data = m;
    jobs->append(new MPCommandJob(mpDev, MPCmd::READ_NOTE_FILE,
                                  noteData,
              [this, jobs](const QByteArray &data, bool &)
                {
                    return readDataNode(jobs, data, false);
                }
              ));

    connect(jobs, &AsyncJobs::finished, [jobs, cb](const QByteArray &)
    {
        //all jobs finished success
        qInfo() << "get_data_node success";
        QVariantMap m = jobs->user_data.toMap();
        QByteArray ndata = m["data"].toByteArray();
        cb(true, QString(), m["note"].toString(), ndata);
    });

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        qCritical() << "Failed getting note node";
        cb(false, failedJob->getErrorStr(), QString(), QByteArray());
    });

    mpDev->enqueueAndRunJob(jobs);
}

bool MPDeviceBleImpl::isNoteAvailable() const
{
    return get_bundleVersion() >= Common::BLE_BUNDLE_WITH_NOTES;
}

void MPDeviceBleImpl::loadNotes(AsyncJobs *jobs, const MPDeviceProgressCb &cbProgress)
{
    if (mpDev->startDataNode[Common::NOTE_ADDR_IDX] != MPNode::EmptyAddress)
    {
        qInfo() << "Loading parent nodes...";
        mpDev->loadDataNode(jobs, mpDev->startDataNode[Common::NOTE_ADDR_IDX], true, cbProgress, Common::NOTE_ADDR_IDX);
    }
    else
    {
        qInfo() << "No parent notes nodes to load.";
    }
}

void MPDeviceBleImpl::checkAndStoreCredential(const BleCredential &cred, MessageHandlerCb cb)
{
    auto *jobs = new AsyncJobs(QString("Check and Store Credential"), this);

    jobs->append(new MPCommandJob(mpDev, MPCmd::CHECK_CREDENTIAL, createCheckCredMessage(cred),
                        [this, cb, jobs, cred] (const QByteArray &data, bool &)
                        {
                            if (MSG_SUCCESS != bleProt->getFirstPayloadByte(data))
                            {
                                storeCredential(cred, cb, jobs);
                            }
                            else
                            {
                                qWarning() << "Credential is already exists";
                                cb(false, "Credential is already exists");
                            }
                            return true;
                        }
               ));

    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::storeCredential(const BleCredential &cred, MessageHandlerCb cb, AsyncJobs *jobs /* =nullptr */)
{
    bool onlyStore = !jobs;
    if (onlyStore)
    {
        jobs = new AsyncJobs(QString("Store Credential"), this);
    }

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
               qWarning() << Common::MMM_CREDENTIAL_STORE_FAILED;
               cb(false, Common::MMM_CREDENTIAL_STORE_FAILED);
           }
           return true;
       }));

    if (onlyStore)
    {
        mpDev->enqueueAndRunJob(jobs);
    }
}

void MPDeviceBleImpl::changePassword(const QByteArray& address, const QString &pwd, MessageHandlerCb cb)
{
    auto* jobs = new AsyncJobs(QString("Change password"), this);

    jobs->prepend(new MPCommandJob(mpDev, MPCmd::SET_NODE_PASSWORD, createChangePasswordMsg(address, pwd),
       [this, cb](const QByteArray &data, bool &)
       {
           if (MSG_SUCCESS == bleProt->getFirstPayloadByte(data))
           {
               qDebug() << "Password changed successfully";
               cb(true, "");
           }
           else
           {
               qWarning() << "Password change failed";
               cb(false, "Password change failed");
           }
           return true;
       }));

    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::getCredential(const QString& service, const QString& login, const QString& reqid, const QString& fallbackService, const MessageHandlerCbData &cb)
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
                            [this, service, login, cb, fallbackService, jobs](const QByteArray &data, bool &)
                            {
                                if (MSG_FAILED == bleProt->getMessageSize(data))
                                {
                                    if (fallbackService.isEmpty())
                                    {
                                        qWarning() << "Credential get failed";
                                        cb(false, "Get credential failed", QByteArray{});
                                    }
                                    else
                                    {
                                        getFallbackServiceCredential(jobs, fallbackService, login, cb);
                                    }
                                    return true;
                                }
                                qDebug() << "Credential got successfully";

                                if (AppDaemon::isDebugDev())
                                    qDebug() << data.toHex();

                                cb(true, "", bleProt->getFullPayload(data));
                                return true;
                            }));

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        qCritical() << "Failed getting credential: " << failedJob->getErrorStr();
        cb(false, failedJob->getErrorStr(), QByteArray{});
    });

    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::getTotpCode(const QString &service, const QString &login, const MessageHandlerCbData &cb)
{
    static constexpr int GET_TOP_CODE_SUPPORT_FW = 8;
    if (get_bundleVersion() < GET_TOP_CODE_SUPPORT_FW)
    {
        cb(false, "Bundle version is not supporting get TOTP code command", QByteArray{});
        return;
    }
    AsyncJobs *jobs = new AsyncJobs("Get TOTP code", this);
    jobs->append(new MPCommandJob(mpDev, MPCmd::GET_TOTP_CODE, createGetCredMessage(service, login),
                            [this, service, login, cb](const QByteArray &data, bool &)
                            {
                                if (MSG_FAILED == bleProt->getMessageSize(data))
                                {
                                    cb(false, "Get TOTP code failed", QByteArray{});
                                    return true;
                                }
                                qDebug() << "TOTP code got successfully";

                                if (AppDaemon::isDebugDev())
                                {
                                    qDebug() << data.toHex();
                                }

                                cb(true, "", bleProt->getFullPayload(data));
                                return true;
                            }));

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        qCritical() << "Failed getting totp code: " << failedJob->getErrorStr();
        cb(false, failedJob->getErrorStr(), QByteArray{});
    });

    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::getFallbackServiceCredential(AsyncJobs *jobs, const QString &fallbackService, const QString &login, const MessageHandlerCbData &cb)
{
    jobs->prepend(new MPCommandJob(mpDev, MPCmd::GET_CREDENTIAL, createGetCredMessage(fallbackService, login),
    [this, cb](const QByteArray &data, bool &)
    {
        if (MSG_FAILED == bleProt->getMessageSize(data))
        {
            qWarning() << "Credential get for fallback service failed";
            cb(false, "Get credential failed", QByteArray{});
        }
        else
        {
            qDebug() << "Credential for fallback service got successfully";
            cb(true, "", bleProt->getFullPayload(data));
        }
        return true;
    }));
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

        lastIndex = index;
        attr = static_cast<BleCredential::CredAttr>(static_cast<int>(attr) + 1);
    }

    return cred;
}

QString MPDeviceBleImpl::retrieveTotpCodeFromResponse(const QByteArray &response)
{
    QString totpCode = "";
    bool isNum = true;
    // Received as ASCII so only need to append every second char for TOTP code
    for (QChar c : response)
    {
        if (isNum)
        {
            totpCode.append(c);
        }
        isNum = !isNum;
    }
    return totpCode;
}

void MPDeviceBleImpl::sendWakeUp()
{
    if (get_bundleVersion() < WAKEUP_DEVICE_BUNDLE_VERSION)
    {
        qWarning() << "Wake up device is not available before bundle version " << WAKEUP_DEVICE_BUNDLE_VERSION;
        return;
    }

    AsyncJobs *jobs = new AsyncJobs("Send Wake Up", this);

    jobs->append(new MPCommandJob(mpDev, MPCmd::WAKE_UP_DEVICE,
                                  [this](const QByteArray &data, bool &)
                                  {
                                      if (MSG_FAILED == bleProt->getMessageSize(data))
                                      {
                                          qWarning() << "Send wake up to device failed";
                                          return false;
                                      }
                                      qDebug() << "Send wake up to device was successfully";

                                      return true;
                                  }));

    mpDev->enqueueAndRunJob(jobs);
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

bool MPDeviceBleImpl::processReceivedData(const QByteArray &data, QByteArray &dataReceived)
{
    const bool isFirst = isFirstPacket(data);
    auto& cmd = mpDev->commandQueue.head();
    if (isFirst)
    {
        cmd.responseSize = bleProt->getMessageSize(data);
        cmd.response.append(data);
        /*
         *  When multiple packet from the device expected
         *  start a timer with 2 sec to detect resets and
         *  clean the command queue if it occurs.
         */
        if (cmd.responseSize > FIRST_PACKET_PAYLOAD_SIZE)
        {
            cmd.timerTimeout = new QTimer(this);
            connect(cmd.timerTimeout, &QTimer::timeout, this, &MPDeviceBleImpl::handleLongMessageTimeout);
            cmd.timerTimeout->setInterval(LONG_MESSAGE_TIMEOUT_MS);
            cmd.timerTimeout->start();
        }
    }

    const bool isLast = isLastPacket(data);
    if (isLast)
    {
        if (!isFirst)
        {
            cmd.timerTimeout->stop();
            /**
             * @brief EXTRA_INFO_SIZE
             * Extra bytes of the first packet.
             * In the last package only the remaining bytes
             * of payload is appended.
             */
            constexpr int EXTRA_INFO_SIZE = 6;
            int fullResponseSize = cmd.responseSize + EXTRA_INFO_SIZE;
            QByteArray responseData = cmd.response;
            if (responseData.size() < fullResponseSize - PAYLOAD_SIZE)
            {
                qDebug() << "Not all packet was received";
                handleLongMessageTimeout();
                return false;
            }
            responseData.append(bleProt->getFullPayload(data).left(fullResponseSize - responseData.size()));
            dataReceived = responseData;
        }
    }
    else
    {
        if (!isFirst)
        {
            cmd.response.append(bleProt->getFullPayload(data));
        }
        cmd.checkReturn = false;
    }
    return isLast;
}

 QVector<QByteArray> MPDeviceBleImpl::processReceivedStartNodes(const QByteArray &data) const
{
    if (data.size() < 2)
    {
        return {QByteArray::number(0)};
    }
    QVector<QByteArray> res;
    for (int i = 0; i < data.size() - 1; i += 2)
    {
        res.append(data.mid(i, 2));
    }
    qDebug() << "Received starting node size: " << res.size();
    if (AppDaemon::isDebugDev())
    {
       qDebug() << "Starting addresses: " << res;
    }
    return res;
 }

 QVector<QByteArray> MPDeviceBleImpl::getDataStartNodes(const QByteArray &data) const
 {
     auto addresses = processReceivedStartNodes(data);
     if (addresses.size() <= FIRST_DATA_STARTING_ADDR)
     {
        qCritical() << "Data starting address is not received";
        return {MPNode::EmptyAddress, MPNode::EmptyAddress};
     }
     auto dataAddresses = addresses.mid(FIRST_DATA_STARTING_ADDR, addresses.size() - FIRST_DATA_STARTING_ADDR);
     qDebug() << "Received data starting node size: " << dataAddresses.size();
     if (AppDaemon::isDebugDev())
     {
        qDebug() << "Data starting addresses: " << dataAddresses;
     }

     return dataAddresses;
 }

bool MPDeviceBleImpl::readDataNode(AsyncJobs *jobs, const QByteArray &data, bool isFile /*= true */)
{
    if (bleProt->getFirstPayloadByte(data) == 0) //Get Data File Chunk fails
    {
        QVariantMap m = jobs->user_data.toMap();
        if (!m.contains("data"))
        {
            jobs->setCurrentJobError("reading data failed or no data");
            return false;
        }
        return true;
    }

    if (bleProt->getMessageSize(data) != 0)
    {
        QVariantMap m = jobs->user_data.toMap();
        QByteArray ba = m["data"].toByteArray();

        QByteArray payload = bleProt->getFullPayload(data);
        QByteArray payloadSize = payload.mid(2, 2);
        quint16 size = qFromLittleEndian<quint16>((quint8*)payloadSize.data());

        bool firstPacket = false;
        if (ba.isEmpty())
        {
            firstPacket = true;
            //If ba is empty it is the first received packet
            m_miniFilePartCounter = size == MINI_FILE_BLOCK_SIZE ? 1 : 0;
            if (m_miniFilePartCounter)
            {
                const int FULL_SIZE_START = 4;
                // Size is received in big endian, but convertToQuint32 expects little endian.
                QByteArray fullSizeArr = Common::reverse(payload.mid(FULL_SIZE_START, MINI_FILE_FULL_SIZE_LENGTH));
                quint32 fullSize = bleProt->convertToQuint32(fullSizeArr);
                m["full_size"] = fullSize;
                m["act_size"] = MINI_FILE_BLOCK_SIZE - MINI_FILE_FULL_SIZE_LENGTH;
            }
        }

        if (0 == size)
        {
            if (!m.contains("data"))
            {
                jobs->setCurrentJobError("reading data failed or no data");
                return false;
            }
            return true;
        }

        if (m_miniFilePartCounter > 1)
        {
            if (m_miniFilePartCounter == 2)
            {
                /**
                 * After a 128B first packet we received an other packet, so it is a mini file
                 * and we can remove 4B size from beggining
                 */
                ba.remove(0, MINI_FILE_FULL_SIZE_LENGTH);
            }
            int fullSize = m["full_size"].toInt();
            int actSize = m["act_size"].toInt();
            if (actSize != fullSize)
            {
                int currentSize = actSize + size;
                if (currentSize > fullSize)
                {
                    /**
                     * When with the actual 128B payload the size is larger than
                     * the full size, we only need the remaining file data from this packet
                     */
                    ba.append(payload.mid(4, fullSize - actSize));
                    m["act_size"] = fullSize;
                }
                else
                {
                    ba.append(payload.mid(4, size));
                    actSize += size;
                    m["act_size"] = actSize;
                }
            }
            m_miniFilePartCounter++;
        }
        else
        {
            ba.append(payload.mid(4, size));
        }

        m["data"] = ba;
        jobs->user_data = m;

        if (firstPacket && m_miniFilePartCounter == 1)
        {
            /**
             * For first packet we increase mini file part counter here, because
             * until the second packet is not received we handle it as BLE file
             */
            ++m_miniFilePartCounter;
        }

        jobs->append(new MPCommandJob(mpDev, isFile ? MPCmd::READ_DATA_FILE : MPCmd::READ_NOTE_FILE,
                  [this, jobs, isFile](const QByteArray &data, bool &)
            {
                return readDataNode(jobs, data, isFile);
            }
          ));
    }
    return true;
}

void MPDeviceBleImpl::createBLEDataChildNodes(MPDevice::ExportPayloadData id, QByteArray& firstAddr, QVector<QByteArray> &miniDataArray)
{
    const int MINI_BLOCK_SIZE = 128;
    const int FLAG_AND_NEXT_ADDR_SIZE = 4;
    const int totalSize = (miniDataArray.size() - 1) * MINI_BLOCK_SIZE + (miniDataArray.last().size() - FLAG_AND_NEXT_ADDR_SIZE);
    int currentSize = 0;
    for (int i = 0; i < miniDataArray.size(); ++i)
    {
        QByteArray addr = firstAddr;
        QByteArray dataPacket = m_bleNodeConverter.convertDataChildNode(miniDataArray[i], totalSize, currentSize, firstAddr);
        /* Create node and add it to the list of imported data nodes */
        MPNode* importedNode = bleProt->createMPNode(qMove(dataPacket), mpDev, qMove(addr), 0);
        mpDev->importNodeMap[id]->append(importedNode);
    }
}

bool MPDeviceBleImpl::hasNextAddress(char addr1, char addr2)
{
    return !(addr1 == ZERO_BYTE && addr2 == ZERO_BYTE);
}

void MPDeviceBleImpl::readExportDataChildNodes(const QJsonArray &nodes, MPDevice::ExportPayloadData id)
{
    for (qint32 i = 0; i < nodes.size(); i++)
    {
        QJsonObject qjobject = nodes[i].toObject();

        /* Fetch address */
        QJsonArray serviceAddrArr = qjobject["address"].toArray();
        QByteArray serviceAddr = QByteArray();
        for (qint32 j = 0; j < serviceAddrArr.size(); j++) {serviceAddr.append(serviceAddrArr[j].toInt());}

        /* Fetch core data */
        QJsonObject dataObj = qjobject["data"].toObject();
        QByteArray dataCore = QByteArray();
        QVector<QByteArray> coreArray;
        for (qint32 j = 0; j < dataObj.size(); j++) {dataCore.append(dataObj[QString::number(j)].toInt());}
        coreArray.append(dataCore);

        if (hasNextAddress(dataCore[NEXT_ADDRESS_STARTING], dataCore[NEXT_ADDRESS_STARTING + 1]))
        {
            /* Collecting data child nodes, while they have next address */
            do
            {
                qjobject = nodes[++i].toObject();
                dataObj = qjobject["data"].toObject();
                dataCore = QByteArray();
                for (qint32 l = 0; l < dataObj.size(); l++) {dataCore.append(dataObj[QString::number(l)].toInt());}
                coreArray.append(dataCore);
            } while (hasNextAddress(dataCore[NEXT_ADDRESS_STARTING], dataCore[NEXT_ADDRESS_STARTING+1]));
        }
        /* Create data child nodes for a given file */
        createBLEDataChildNodes(id, serviceAddr, coreArray);
    }
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

    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::setUserCategories(const QJsonObject &categories, const MessageHandlerCbData &cb)
{
    AsyncJobs *jobs = new AsyncJobs("Set User Categories", this);

    jobs->append(new MPCommandJob(mpDev, MPCmd::SET_USER_CATEGORIES,
                                  createUserCategoriesMsg(categories),
                            [this, cb, categories](const QByteArray &data, bool &)
                            {
                                if (MSG_FAILED == bleProt->getFirstPayloadByte(data))
                                {
                                    qWarning() << "Set user categories failed";
                                    cb(false, "Set user categories failed", QByteArray{});
                                    return true;
                                }
                                m_categories = categories;
                                qDebug() << "User categories set successfully";
                                cb(true, "", QByteArray{});
                                return true;
                            }));

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        qCritical() << "Failed setting user categories: " << failedJob->getErrorStr();
        cb(false, failedJob->getErrorStr(), QByteArray{});
    });

    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::fillGetCategory(const QByteArray& data, QJsonObject &categories)
{
    for (int i = 0; i < USER_CATEGORY_COUNT; ++i)
    {
        QString catName = "category_" + QString::number(i+1);
        QString category = bleProt->toQString(data.mid(i*USER_CATEGORY_LENGTH, (i+1)*USER_CATEGORY_LENGTH));
        bool defaultCat = std::all_of(std::begin(category), std::end(category),
                                   [](const QChar& c) { return c == QChar{0xFFFF};});
        categories[catName] = defaultCat ? catName : category;
    }
    m_categories = categories;
    m_categoriesFetched = true;
}

void MPDeviceBleImpl::fetchCategories()
{
    if (!m_categoriesFetched && Common::Unlocked == mpDev->get_status())
    {
        getUserCategories([this](bool success, QString, QByteArray data)
                         {
                             if (success)
                             {
                                QJsonObject ores;
                                fillGetCategory(data, ores);
                                emit userCategoriesFetched(ores);
                             }
                         });
    }
}

void MPDeviceBleImpl::handleFirstBluetoothMessageTimeout()
{
    if (AppDaemon::isDebugDev())
    {
        qDebug() << "handleFirstBluetoothMessageTimeout";
    }
    auto& cmd = mpDev->commandQueue.head();
    delete cmd.timerTimeout;
    cmd.timerTimeout = nullptr;
    cmd.running = false;
    sendResetFlipBit();
    mpDev->sendDataDequeue();
}

QByteArray MPDeviceBleImpl::createUserCategoriesMsg(const QJsonObject &categories)
{
    QByteArray data;
    for (int i = 0; i < USER_CATEGORY_COUNT; ++i)
    {
        QByteArray categoryArr = bleProt->toByteArray(categories["category_" + QString::number(i+1)].toString());
        Common::fill(categoryArr, USER_CATEGORY_LENGTH - categoryArr.size(), ZERO_BYTE);
        data.append(categoryArr);
    }
    return data;
}

void MPDeviceBleImpl::createTOTPCredMessage(const QString &service, const QString &login, const QJsonObject &totp)
{
    QByteArray data;
    QByteArray secretKey = Base32::decode(totp["totp_secret_key"].toString());
    int timeStep = totp["totp_time_step"].toInt();
    int codeSize = totp["totp_code_size"].toInt();

    // 0->251 Service name
    QByteArray serviceArr = bleProt->toByteArray(service);
    Common::fill(serviceArr, MPNodeBLE::SERVICE_LENGTH - serviceArr.size(), ZERO_BYTE);
    data.append(serviceArr);
    // 252->379 Login name
    QByteArray loginArr = bleProt->toByteArray(login);
    Common::fill(loginArr, MPNodeBLE::LOGIN_LENGTH - loginArr.size(), ZERO_BYTE);
    data.append(loginArr);
    // 380->443 TOTP secret key
    QByteArray secKeyArr;
    secKeyArr.append(secretKey);
    if (secKeyArr.length() > SECRET_KEY_LENGTH)
    {
        qCritical() << "Secretkey is longer than " << SECRET_KEY_LENGTH;
        secKeyArr = secKeyArr.left(SECRET_KEY_LENGTH);
    }
    Common::fill(secKeyArr, SECRET_KEY_LENGTH - secKeyArr.size(), ZERO_BYTE);
    data.append(secKeyArr);
    // 444 TOTP secret key length
    data.append(secretKey.size());
    // 445 TOTP number of digits
    data.append(codeSize);
    // 456 TOTP time step
    data.append(timeStep);
    // 447 TOTP SHA version
    data.append(ZERO_BYTE);

    mmmTOTPStoreArray.append(data);
}

bool MPDeviceBleImpl::storeTOTPCreds()
{
    if (mmmTOTPStoreArray.isEmpty())
    {
        return false;
    }
    AsyncJobs *totpStoreJob = new AsyncJobs("Storing TOTP Credentials...", this);
    for (QByteArray& totpMsg : mmmTOTPStoreArray)
    {
        totpStoreJob->append(new MPCommandJob(mpDev, MPCmd::STORE_TOTP_CRED,
                                      totpMsg,
                                      [this](const QByteArray &data, bool &) -> bool
        {
            if (bleProt->getFirstPayloadByte(data) == MSG_FAILED)
            {
                mpDev->currentJobs->setCurrentJobError("store_totp_cred failed on device");
                qWarning() << "Failed to set TOTP Credential";
                return false;
            }
            else
            {
                qDebug() << "Stored TOTP Credential succesfully";
                return true;
            }
        }));
    }

    connect(totpStoreJob, &AsyncJobs::finished, [this](const QByteArray &)
    {
        qInfo() << "TOTP Credentials added!";
        mmmTOTPStoreArray.clear();
    });

    connect(totpStoreJob, &AsyncJobs::failed, [this](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob)
        mmmTOTPStoreArray.clear();
        qCritical() << "Couldn't change setup/update TOTP Credential";
    });

    mpDev->jobsQueue.enqueue(totpStoreJob);
    return true;
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
        set_knockDisabled(d&KNOCK_DISABLED);
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
    settingJson["knock_disabled"] = get_knockDisabled();
    emit userSettingsChanged(settingJson);
}

void MPDeviceBleImpl::readBatteryPercent(const QByteArray& statusData)
{
    if (STATUS_MSG_SIZE_WITH_BATTERY == bleProt->getMessageSize(statusData))
    {
        quint8 batteryPct = bleProt->getPayloadByteAt(statusData, BATTERY_BYTE);
        bool charging = batteryPct&BATTERY_CHARGING_BIT;
        if (charging)
        {
            // Unset charging bit for battery percent
            batteryPct &= ~(BATTERY_CHARGING_BIT);
        }
        set_chargingStatus(charging);
        qDebug() << "Battery percent: " << batteryPct;
        if (batteryPct != m_battery)
        {
            m_battery = batteryPct;
            emit batteryPercentChanged(m_battery);
        }
    }
}

void MPDeviceBleImpl::getBattery()
{
    if (m_battery != INVALID_BATTERY)
    {
        emit batteryPercentChanged(m_battery);
    }
}

void MPDeviceBleImpl::nihmReconditioning(bool enableRestart, int ratedCapacity)
{
    auto *jobs = new AsyncJobs("NiMH Reconditioning", mpDev);
    m_nimhResultSec = 0;

    /* This is just proporional calculation */
    m_nimhRestartUnderSec = RECONDITION_RESTART_UNDER_SECS_STOCK * 300 / ratedCapacity;
    auto restartUnder = m_nimhRestartUnderSec;

    jobs->append(new MPCommandJob(mpDev, MPCmd::NIMH_RECONDITION, [this](const QByteArray &data, bool &)
    {
        const auto payload = bleProt->getFullPayload(data);
        if (RECONDITION_RESPONSE_SIZE != payload.size())
        {
            qWarning() << "Invalid nimh recondition response size";
            return false;
        }
        const auto dischargeTimeLower = bleProt->toIntFromLittleEndian(static_cast<quint8>(payload[0]), static_cast<quint8>(payload[1]));
        const auto dischargeTimeUpper = bleProt->toIntFromLittleEndian(static_cast<quint8>(payload[2]), static_cast<quint8>(payload[3]));
        quint32 dischargeTime = dischargeTimeLower;
        dischargeTime |= static_cast<quint32>((dischargeTimeUpper<<16));
        m_nimhResultSec = dischargeTime/1000.0;
        if (std::numeric_limits<quint32>::max() == dischargeTime)
        {
            qCritical() << "NiMH Recondition failed";
            return false;
        }
        return true;
    }));

    connect(jobs, &AsyncJobs::finished, [this, enableRestart, restartUnder, ratedCapacity](const QByteArray &data)
    {
        Q_UNUSED(data)
        qDebug() << "NiMH Reconditioning finished";
        bool restarted = false;
        if (enableRestart && m_nimhResultSec < restartUnder)
        {
            restarted = true;
        }
        createAndAddCustomJob("NiMH Reconditioning finished job",
                              [this, restarted, result = m_nimhResultSec](){emit nimhReconditionFinished(true, QString::number(result), restarted);});
        if (restarted)
        {
            nihmReconditioning(true, ratedCapacity);
        }
    });

    connect(jobs, &AsyncJobs::failed, [this](AsyncJob *)
    {
        qDebug() << "NiMH Reconditioning failed";
        createAndAddCustomJob("NiMH Reconditioning failed job",
                        [this](){emit nimhReconditionFinished(false, QString::number(m_nimhResultSec));});
    });

    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::getSecurityChallenge(const QString &key, const MessageHandlerCb &cb)
{
    AsyncJobs *jobs = new AsyncJobs("Send security challenge to device", this);

    jobs->append(new MPCommandJob(mpDev, MPCmd::AUTH_CHALLENGE,
                                Common::toHexArray(key),
                                [this, cb](const QByteArray &data, bool &)
    {
        if (ERROR_MSG_SIZE == bleProt->getMessageSize(data))
        {
            qWarning() << "Authentication challenge failed";
            cb(false, "Authentication challenge failed");
            return false;
        }
        cb(true, Common::toHexString(bleProt->getFullPayload(data)));
        return true;
    }));

    connect(jobs, &AsyncJobs::failed, [](AsyncJob *)
    {
        qWarning() << "Failed get uid from device";
    });

    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::processDebugMsg(const QByteArray &data, bool &isDebugMsg)
{
    if (isFirstPacket(data))
    {
        isDebugMsg = true;
    }

    m_debugMsg += bleProt->toQString(bleProt->getFullPayload(data));
    if (isLastPacket(data))
    {
        qWarning() << m_debugMsg;
        isDebugMsg = false;
        m_debugMsg.clear();
    }
}

void MPDeviceBleImpl::updateChangeNumbers(AsyncJobs *jobs, quint8 flags)
{
    if (flags&Common::CredentialNumberChanged)
    {
        jobs->append(new MPCommandJob(mpDev, MPCmd::SET_USER_CHANGE_NB,
                                      bleProt->toLittleEndianFromInt32(mpDev->get_credentialsDbChangeNumber()),
                                      bleProt->getDefaultFuncDone()));
    }

    if (flags&Common::DataNumberChanged)
    {
        jobs->append(new MPCommandJob(mpDev, MPCmd::SET_DATA_CHANGE_NB,
                                      bleProt->toLittleEndianFromInt32(mpDev->get_dataDbChangeNumber()),
                                      bleProt->getDefaultFuncDone()));
    }
}

void MPDeviceBleImpl::updateUserCategories(const QJsonObject &categories)
{
    if (isUserCategoriesChanged(categories))
    {
        qDebug() << "Updating user category names";
        setUserCategories(categories,
                          [](bool success, QString errstr, QByteArray)
                            {
                                if (!success)
                                {
                                    qCritical() << "Update user categories failed: " << errstr;
                                }
                            }
        );
    }
}

bool MPDeviceBleImpl::isUserCategoriesChanged(const QJsonObject &categories) const
{
    for (int i = 0; i < USER_CATEGORY_COUNT; ++i)
    {
        QString categoryName = "category_" + QString::number(i+1);
        if (categories[categoryName] != m_categories[categoryName])
        {
            return true;
        }
    }
    return false;
}

void MPDeviceBleImpl::setImportUserCategories(const QJsonObject &categories)
{
    m_categoriesToImport = categories;
}

void MPDeviceBleImpl::importUserCategories()
{
    if (m_categoriesToImport.empty())
    {
        qCritical() << "Cannot import empty categories";
        return;
    }
    updateUserCategories(m_categoriesToImport);
    m_categoriesToImport = QJsonObject();
}

void MPDeviceBleImpl::setNodeCategory(MPNode* node, int category)
{
    if (auto* nodeBle = dynamic_cast<MPNodeBLE*>(node))
    {
        if (AppDaemon::isDebugDev())
        {
            qDebug() << "Setting category to: " << category;
        }
        nodeBle->setCategory(category);
    }
}

void MPDeviceBleImpl::setNodeKeyAfterLogin(MPNode *node, int key)
{
    if (auto* nodeBle = dynamic_cast<MPNodeBLE*>(node))
    {
        if (AppDaemon::isDebugDev())
        {
            qDebug() << "Setting keyAfterLogin to: " << key;
        }
        nodeBle->setKeyAfterLogin(key);
    }
}

void MPDeviceBleImpl::setNodeKeyAfterPwd(MPNode *node, int key)
{
    if (auto* nodeBle = dynamic_cast<MPNodeBLE*>(node))
    {
        if (AppDaemon::isDebugDev())
        {
            qDebug() << "Setting keyAfterPwd to: " << key;
        }
        nodeBle->setKeyAfterPwd(key);
    }
}

void MPDeviceBleImpl::setNodePwdBlankFlag(MPNode *node)
{
    if (auto* nodeBle = dynamic_cast<MPNodeBLE*>(node))
    {
        if (AppDaemon::isDebugDev())
        {
            qDebug() << "Setting password blank flag";
        }
        nodeBle->setPwdBlankFlag();
    }
}

bool MPDeviceBleImpl::setNodePointedToAddr(MPNode *node, QByteArray addr)
{
    if (addr.size() != POINTED_TO_ADDR_SIZE)
    {
        // Pointed to is not a valid node address
        return false;
    }
    if (addr.at(0) == ZERO_BYTE && addr.at(1) == ZERO_BYTE)
    {
        // Not pointed to other node
        return false;
    }
    if (auto* nodeBle = dynamic_cast<MPNodeBLE*>(node))
    {
        nodeBle->setPointedToChildAddr(addr);
        return true;
    }
    return false;
}

bool MPDeviceBleImpl::setNodeMultipleDomains(MPNode *node, const QString &domains)
{
    bool changed = false;
    if (auto* nodeBle = dynamic_cast<MPNodeBLE*>(node))
    {
        QString oldDomains = nodeBle->getMultipleDomains();
        if (AppDaemon::isDebugDev())
        {
            qDebug() << "Setting multiple domains to: " << domains;
        }
        changed = nodeBle->setMultipleDomains(domains);
    }
    return changed;
}

QList<QByteArray> MPDeviceBleImpl::getFavorites(const QByteArray &data)
{
    QList<QByteArray> res;
    int start = 0;
    int end = FAV_DATA_SIZE;
    while (res.size() < FAV_NUMBER)
    {
        res.append(bleProt->getPayloadBytes(data, start, end));
        start = end;
        end += FAV_DATA_SIZE;
    }
    return res;
}

QByteArray MPDeviceBleImpl::getStartAddressToSet(const QVector<QByteArray>& startNodeArray, Common::AddressType addrType) const
{
    QByteArray setAddress;
    setAddress.append(static_cast<char>(addrType));
    setAddress.append(ZERO_BYTE);
    setAddress.append(startNodeArray[addrType]);
    return setAddress;
}

void MPDeviceBleImpl::readLanguages(bool onlyCheck, const MessageHandlerCb &cb)
{
    m_deviceLanguages = QJsonObject{};
    m_keyboardLayouts = QJsonObject{};
    AsyncJobs *jobs = new AsyncJobs(
                          "Read languages",
                          this);
    jobs->append(new MPCommandJob(mpDev,
                   MPCmd::GET_LANG_NUM,
                   [this, jobs, onlyCheck] (const QByteArray &data, bool &)
                    {
                        const auto payload = bleProt->getFullPayload(data);
                        const int langNum = bleProt->toIntFromLittleEndian(payload[0], payload[1]);
                        qDebug() << "Language number: " << langNum;
                        if (INVALID_LAYOUT_LANG_SIZE == langNum)
                        {
                            qCritical() << "Invalid number of languages";
                            return false;
                        }
                        if (s_LangNum == langNum && onlyCheck)
                        {
                            qDebug() << "No need to fetch languages, it is already up-to-date.";
                            return true;
                        }
                        else
                        {
                            s_LangNum = langNum;
                        }
                        for (int i = 0; i < langNum; ++i)
                        {
                            jobs->append(new MPCommandJob(mpDev,
                                           MPCmd::GET_LANG_DESC,
                                           QByteArray(1, static_cast<char>(i)),
                                           [this, i, langNum] (const QByteArray &data, bool &)
                                            {
                                                const QString lang = bleProt->toQString(bleProt->getFullPayload(data));
                                                if (AppDaemon::isDebugDev())
                                                {
                                                    qDebug() << i << " lang desc: " << lang;
                                                }
                                                m_deviceLanguages[lang] = i;
                                                if (i == (langNum - 1))
                                                {
                                                    emit bleDeviceLanguage(m_deviceLanguages);
                                                }
                                                return true;
                                            }
                            ));
                        }
                        return true;
                    }
    ));
    jobs->append(new MPCommandJob(mpDev,
                   MPCmd::GET_KEYB_LAYOUT_NUM,
                   [this, jobs, onlyCheck, cb] (const QByteArray &data, bool &)
                    {
                        const auto payload = bleProt->getFullPayload(data);
                        const auto layoutNum = bleProt->toIntFromLittleEndian(payload[0], payload[1]);
                        qDebug() << "Keyboard layout number: " << layoutNum;
                        if (INVALID_LAYOUT_LANG_SIZE == layoutNum)
                        {
                            qCritical() << "Invalid number of keyboard layouts";
                            cb(false, "");
                            return false;
                        }
                        if (s_LayoutNum == layoutNum && onlyCheck)
                        {
                            qDebug() << "No need to fetch layouts, it is already up-to-date.";
                            cb(true, "");
                            return true;
                        }
                        else
                        {
                            s_LayoutNum = layoutNum;
                        }
                        for (int i = 0; i < layoutNum; ++i)
                        {
                            jobs->append(new MPCommandJob(mpDev,
                                           MPCmd::GET_LAYOUT_DESC,
                                           QByteArray(1, static_cast<char>(i)),
                                           [this, i, layoutNum] (const QByteArray &data, bool &)
                                            {
                                                const QString layout = bleProt->toQString(bleProt->getFullPayload(data));
                                                if (AppDaemon::isDebugDev())
                                                {
                                                    qDebug() << i << " layout desc: " << layout;
                                                }
                                                m_keyboardLayouts[layout] = i;
                                                if (i == (layoutNum - 1))
                                                {
                                                    emit bleKeyboardLayout(m_keyboardLayouts);
                                                }
                                                return true;
                                            }
                            ));
                        }
                        return true;
                    }
    ));

    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::loadWebAuthnNodes(AsyncJobs * jobs, const MPDeviceProgressCb &cbProgress)
{
    if (mpDev->startNode[Common::WEBAUTHN_ADDR_IDX] != MPNode::EmptyAddress)
    {
        qInfo() << "Loading parent nodes...";
        mpDev->loadLoginNode(jobs, mpDev->startNode[Common::WEBAUTHN_ADDR_IDX], cbProgress, Common::WEBAUTHN_ADDR_IDX);
    }
    else
    {
        qInfo() << "No parent webauthn nodes to load.";
    }
}

void MPDeviceBleImpl::appendLoginNode(MPNode *loginNode, MPNode *loginNodeClone, Common::AddressType addrType)
{
    switch(addrType)
    {
        case Common::CRED_ADDR_IDX:
            mpDev->loginNodes.append(loginNode);
            mpDev->loginNodesClone.append(loginNodeClone);
            break;
        case Common::WEBAUTHN_ADDR_IDX:
            mpDev->webAuthnLoginNodes.append(loginNode);
            mpDev->webAuthnLoginNodesClone.append(loginNodeClone);
            break;
        default:
            qCritical() << "Invalid address type";
    }
}

void MPDeviceBleImpl::appendLoginChildNode(MPNode *loginChildNode, MPNode *loginChildNodeClone, Common::AddressType addrType)
{
    switch(addrType)
    {
        case Common::CRED_ADDR_IDX:
            mpDev->loginChildNodes.append(loginChildNode);
            mpDev->loginChildNodesClone.append(loginChildNodeClone);
            break;
        case Common::WEBAUTHN_ADDR_IDX:
            mpDev->webAuthnLoginChildNodes.append(loginChildNode);
            mpDev->webAuthnLoginChildNodesClone.append(loginChildNodeClone);
            break;
        default:
            qCritical() << "Invalid address type";
    }
}

void MPDeviceBleImpl::appendDataNode(MPNode *loginNode, MPNode *loginNodeClone, Common::DataAddressType addrType)
{
    switch(addrType)
    {
        case Common::DATA_ADDR_IDX:
            mpDev->dataNodes.append(loginNode);
            mpDev->dataNodesClone.append(loginNodeClone);
            break;
        case Common::NOTE_ADDR_IDX:
            mpDev->notesLoginNodes.append(loginNode);
            mpDev->notesLoginNodesClone.append(loginNodeClone);
            break;
        default:
            qCritical() << "Invalid address type";
    }
}

void MPDeviceBleImpl::appendDataChildNode(MPNode *loginChildNode, MPNode *loginChildNodeClone, Common::DataAddressType addrType)
{
    switch(addrType)
    {
        case Common::DATA_ADDR_IDX:
            mpDev->dataChildNodes.append(loginChildNode);
            mpDev->dataChildNodesClone.append(loginChildNodeClone);
            break;
        case Common::NOTE_ADDR_IDX:
            mpDev->notesLoginChildNodes.append(loginChildNode);
            mpDev->notesLoginChildNodesClone.append(loginChildNodeClone);
            break;
        default:
            qCritical() << "Invalid address type";
    }
}

void MPDeviceBleImpl::generateExportData(QJsonArray &exportTopArray)
{
    /* isBle */
    exportTopArray.append(QJsonValue{true});
    /* user category names */
    exportTopArray.append(getUserCategories());
    /* Webauthn parent nodes */
    QJsonArray nodeQJsonArray = QJsonArray();
    auto& login = mpDev->webAuthnLoginNodes;
    for (qint32 i = 0; i < login.size(); i++)
    {
        QJsonObject nodeObject = QJsonObject();
        nodeObject["address"] = QJsonValue(Common::bytesToJson(login[i]->getAddress()));
        nodeObject["name"] = QJsonValue(login[i]->getService());
        nodeObject["data"] = QJsonValue(Common::bytesToJsonObjectArray(login[i]->getNodeData()));
        nodeQJsonArray.append(QJsonValue(nodeObject));
    }
    exportTopArray.append(QJsonValue(nodeQJsonArray));

    /* Webauthn child nodes */
    nodeQJsonArray = QJsonArray();
    auto& loginChild = mpDev->webAuthnLoginChildNodes;
    for (qint32 i = 0; i < loginChild.size(); i++)
    {
        QJsonObject nodeObject = QJsonObject();
        nodeObject["address"] = QJsonValue(Common::bytesToJson(loginChild[i]->getAddress()));
        nodeObject["name"] = QJsonValue(loginChild[i]->getLogin());
        nodeObject["data"] = QJsonValue(Common::bytesToJsonObjectArray(loginChild[i]->getNodeData()));
        nodeObject["pointed"] = QJsonValue(false);
        nodeQJsonArray.append(QJsonValue(nodeObject));
    }
    exportTopArray.append(QJsonValue(nodeQJsonArray));
    exportTopArray.append(QJsonValue(m_currentUserSettings));
    auto* bleSettings = static_cast<DeviceSettingsBLE*>(mpDev->settings());
    exportTopArray.append(QJsonValue(bleSettings->get_user_language()));
    exportTopArray.append(QJsonValue(bleSettings->get_keyboard_bt_layout()));
    exportTopArray.append(QJsonValue(bleSettings->get_keyboard_usb_layout()));

    if (isNoteAvailable())
    {
        /* Notes parent nodes */
        QJsonArray notesJsonArray = QJsonArray{};
        auto& note = mpDev->notesLoginNodes;
        for (qint32 i = 0; i < note.size(); i++)
        {
            QJsonObject noteObject = QJsonObject();
            noteObject["address"] = QJsonValue(Common::bytesToJson(note[i]->getAddress()));
            noteObject["name"] = QJsonValue(note[i]->getService());
            noteObject["data"] = QJsonValue(Common::bytesToJsonObjectArray(note[i]->getNodeData()));
            notesJsonArray.append(QJsonValue(noteObject));
        }
        exportTopArray.append(QJsonValue(notesJsonArray));
        /* Notes child nodes */
        notesJsonArray = QJsonArray{};
        auto& noteChild = mpDev->notesLoginChildNodes;
        for (qint32 i = 0; i < noteChild.size(); i++)
        {
            QJsonObject noteChildObject = QJsonObject();
            noteChildObject["address"] = QJsonValue(Common::bytesToJson(noteChild[i]->getAddress()));
            noteChildObject["data"] = QJsonValue(Common::bytesToJsonObjectArray(noteChild[i]->getNodeData()));
            noteChildObject["pointed"] = QJsonValue(false);
            notesJsonArray.append(QJsonValue(noteChildObject));
        }
        exportTopArray.append(QJsonValue(notesJsonArray));
    }
}

void MPDeviceBleImpl::addUnknownCardPayload(const QJsonValue &val)
{
    mpDev->unknownCardAddPayload.append(toChar(val));
    mpDev->unknownCardAddPayload.append(ZERO_BYTE);
}

void MPDeviceBleImpl::fillAddUnknownCard(const QJsonArray &dataArray)
{
    addUnknownCardPayload(dataArray[MPDevice::EXPORT_SECURITY_SETTINGS_INDEX]);
    addUnknownCardPayload(dataArray[MPDevice::EXPORT_USER_LANG_INDEX]);
    addUnknownCardPayload(dataArray[MPDevice::EXPORT_BT_LAYOUT_INDEX]);
    addUnknownCardPayload(dataArray[MPDevice::EXPORT_USB_LAYOUT_INDEX]);
}

void MPDeviceBleImpl::addUserIdPlaceholder(QByteArray &array)
{
    Common::fill(array, 2, ZERO_BYTE);
}

void MPDeviceBleImpl::fillMiniExportPayload(QByteArray &unknownCardPayload)
{
    const auto payloadSize = unknownCardPayload.size();
    if (payloadSize >= UNKNOWN_CARD_PAYLOAD_SIZE)
    {
        return;
    }
    Common::fill(unknownCardPayload, UNKNOWN_CARD_PAYLOAD_SIZE - payloadSize, ZERO_BYTE);
}

void MPDeviceBleImpl::convertMiniToBleNode(QByteArray &array, bool isData)
{
    m_bleNodeConverter.convert(array, isData);
}

void MPDeviceBleImpl::storeFileData(int current, AsyncJobs *jobs, const MPDeviceProgressCb &cbProgress, bool isFile)
{
    QByteArray packet;
    // 4B Set to 0
    const int BYTES_TO_ZERO_SIZE = 4;
    Common::fill(packet, BYTES_TO_ZERO_SIZE, ZERO_BYTE);

    auto& currentDataNode = mpDev->currentDataNode;
    const auto currentNodeSize = currentDataNode.size();
    int currentSize = currentNodeSize - current;
    bool moreChunk = false;
    if (currentSize > BLE_DATA_BLOCK_SIZE)
    {
        currentSize = BLE_DATA_BLOCK_SIZE;
        moreChunk = true;
    }
    // Amount of bytes in this packet (from 0 to 512)
    QByteArray currentSizeArr;
    currentSizeArr.resize(2);
    qToLittleEndian(currentSize, currentSizeArr.data());
    packet.append(currentSizeArr);

    // First (up to) 256B of data to store
    QByteArray firstChunkData = currentDataNode.mid(current, BLE_DATA_BLOCK_SIZE/2);
    firstChunkData.resize(BLE_DATA_BLOCK_SIZE/2);
    packet.append(firstChunkData);

    // 4B Set to 0
    Common::fill(packet, BYTES_TO_ZERO_SIZE, ZERO_BYTE);

    // Second (up to) 256B of data to store
    QByteArray secondChunkData = currentDataNode.mid(current + BLE_DATA_BLOCK_SIZE/2, BLE_DATA_BLOCK_SIZE/2);
    secondChunkData.resize(BLE_DATA_BLOCK_SIZE/2);
    packet.append(secondChunkData);

    // Total file size
    QByteArray totalSize;
    const int TOTAL_SIZE_LENGTH = 4;
    totalSize.resize(TOTAL_SIZE_LENGTH);
    qToLittleEndian(currentNodeSize, totalSize.data());
    packet.append(totalSize);

    // 0 to signal upcoming data, otherwise 1 to signal last packet
    packet.append(static_cast<char>(moreChunk ? 1 : 0));
    packet.append(ZERO_BYTE);
    MPCmd::Command writeCommand = isFile ? MPCmd::WRITE_DATA_FILE : MPCmd::WRITE_NOTE_FILE;
    jobs->append(new MPCommandJob(mpDev, writeCommand,
              packet,
              [this, jobs, cbProgress, current, moreChunk, isFile](const QByteArray &data, bool &)
                {
                    if (bleProt->getFirstPayloadByte(data) != 1)
                    {
                        qCritical() << "Cannot write data to device";
                        return false;
                    }

                    if (moreChunk)
                    {
                        storeFileData(current+BLE_DATA_BLOCK_SIZE, jobs, cbProgress, isFile);
                    }
                    return true;
                }
              ));

    if (isFile)
    {
        QVariantMap cbData = {
            {"total", currentNodeSize},
            {"current", current + BLE_DATA_BLOCK_SIZE},
            {"msg", "WORKING on setDataNodeCb"}
        };
        cbProgress(cbData);
    }
}

void MPDeviceBleImpl::sendInitialStatusRequest()
{
    auto *statusJob = new AsyncJobs(QString("Getting initial status"), this);
    statusJob->append(new MPCommandJob(mpDev, MPCmd::MOOLTIPASS_STATUS, [this](const QByteArray &data, bool &)
    {
        mpDev->processStatusChange(data);
        return true;
    }));
    mpDev->enqueueAndRunJob(statusJob);
}

void MPDeviceBleImpl::checkNoBundle(Common::MPStatus& status, Common::MPStatus prevStatus)
{
    if (status != Common::UnknownStatus &&
            status&Common::NoBundle)
    {
        m_noBundle = true;
        if (status != Common::NoBundle)
        {
            status = Common::NoBundle;
        }
    }
    if (prevStatus == Common::NoBundle)
    {
        m_noBundle = false;
    }
}

bool MPDeviceBleImpl::isNoBundle(MPCmd::Command cmd)
{
    if (m_noBundle && !m_noBundleCommands.contains(cmd))
    {
        mpDev->currentJobs->failCurrent();
        mpDev->commandQueue.dequeue();
        mpDev->sendDataDequeue();
        return true;
    }
    return false;
}

void MPDeviceBleImpl::handleFirstBluetoothMessage(MPCommand &cmd)
{
    cmd.timerTimeout = new QTimer(this);
    connect(cmd.timerTimeout, &QTimer::timeout, this, &MPDeviceBleImpl::handleFirstBluetoothMessageTimeout);
    cmd.timerTimeout->setInterval(BLUETOOTH_FIRST_MSG_TIMEOUT_MS);
    cmd.timerTimeout->start();
    m_isFirstMessageWritten = true;
}

void MPDeviceBleImpl::enforceLayout()
{
    if (!m_enforceLayout)
    {
        return;
    }
    QSettings s;
    bool btLayoutEnforced = s.value(Common::SETTING_BT_LAYOUT_ENFORCE, false).toBool();
    bool usbLayoutEnforced = s.value(Common::SETTING_USB_LAYOUT_ENFORCE, false).toBool();
    if (!btLayoutEnforced && !usbLayoutEnforced)
    {
        return;
    }
    AsyncJobs *jobs = new AsyncJobs("Enforce layout", mpDev);
    if (usbLayoutEnforced)
    {
        QByteArray layoutUsbData;
        layoutUsbData.append(DeviceSettingsBLE::USB_LAYOUT_ID);
        layoutUsbData.append(s.value(Common::SETTING_USB_LAYOUT_ENFORCE_VALUE).toInt());
        jobs->append(new MPCommandJob(mpDev,
                       MPCmd::SET_TMP_KEYB_LAYOUT,
                       layoutUsbData,
                       bleProt->getDefaultFuncDone()
        ));
    }

    if (btLayoutEnforced)
    {
        QByteArray layoutBtData;
        layoutBtData.append(DeviceSettingsBLE::BT_LAYOUT_ID);
        layoutBtData.append(s.value(Common::SETTING_BT_LAYOUT_ENFORCE_VALUE).toInt());
        jobs->append(new MPCommandJob(mpDev,
                       MPCmd::SET_TMP_KEYB_LAYOUT,
                       layoutBtData,
                       bleProt->getDefaultFuncDone()
        ));
    }

    mpDev->enqueueAndRunJob(jobs);
    m_enforceLayout = false;
}

bool MPDeviceBleImpl::resetDefaultSettings()
{
    auto* bleSettings = static_cast<DeviceSettingsBLE*>(mpDev->settings());
    if (!bleSettings)
    {
        qCritical() << "Invalid device settings";
        return false;
    }

    bleSettings->resetDefaultSettings();
    if (get_bundleVersion() >= SET_BLE_NAME_BUNDLE_VERSION)
    {
        setBleName(DEFAULT_BLE_NAME, [this](bool success)
            {
                if (success)
                {
                    emit changeBleName(DEFAULT_BLE_NAME);
                }
                else
                {
                    qCritical() << "Setting default Device Bluetooth Name failed";
                }
            });
    }
    return true;
}

void MPDeviceBleImpl::setBleName(QString name, std::function<void (bool)> cb)
{
    QByteArray nameArray = name.toUtf8();
    nameArray.append(ZERO_BYTE);
    auto *jobs = new AsyncJobs(QString("Set BLE name"), this);
    jobs->append(new MPCommandJob(mpDev, MPCmd::SET_BLE_NAME,
                                  nameArray,
                                  [this, name, cb](const QByteArray &data, bool &) -> bool
    {
        if (bleProt->getFirstPayloadByte(data) != MSG_SUCCESS)
        {
            qWarning() << "Set ble name to: " << name << " failed";
            cb(false);
            return false;
        }
        cb(true);
        return true;
    }));
    mpDev->enqueueAndRunJob(jobs);
}

void MPDeviceBleImpl::getBleName(const MessageHandlerCbData &cb)
{
    auto *jobs = new AsyncJobs("Get Ble Name", mpDev);

    jobs->append(new MPCommandJob(mpDev, MPCmd::GET_BLE_NAME, bleProt->getDefaultSizeCheckFuncDone()));

    connect(jobs, &AsyncJobs::finished, [this, cb](const QByteArray &data)
    {
        Q_UNUSED(data);
        QByteArray bleNameArr = bleProt->getFullPayload(data);
        if (ZERO_BYTE == bleNameArr[0] || static_cast<char>(0xFF) == bleNameArr[0])
        {
            // Handle invalid character in BLE name (starts with 0x00 or 0xFF)
            bleNameArr = DEFAULT_BLE_NAME.toUtf8();
            qWarning() << "Invalid character in BLE name";
        }
        bleNameArr = Common::getUntilNullByte(bleNameArr);
        cb(true, "", bleNameArr);
    });

    mpDev->enqueueAndRunJob(jobs);
}

QString MPDeviceBleImpl::getBleNameFromArray(const QByteArray &arr) const
{
    return bleProt->toQString(arr);
}

void MPDeviceBleImpl::setSerialNumber(quint32 serialNum, std::function<void (bool)> cb)
{
    QByteArray serialNumArr = bleProt->toLittleEndianFromInt32(serialNum);
    auto *jobs = new AsyncJobs(QString("Set Serial number"), this);
    jobs->append(new MPCommandJob(mpDev, MPCmd::SET_SERIAL_NUMBER,
                                  serialNumArr,
                                  [this, serialNum, cb](const QByteArray &data, bool &) -> bool
    {
        if (bleProt->getFirstPayloadByte(data) != MSG_SUCCESS)
        {
            qWarning() << "Set serial number to: " << serialNum << " failed";
            cb(false);
            return false;
        }
        cb(true);
        return true;
    }));
    mpDev->enqueueAndRunJob(jobs);
}

Common::SubdomainSelection MPDeviceBleImpl::getForceSubdomainSelection() const
{
    QSettings s;
    if (get_bundleVersion() >= FORCE_SUBDOMAIN_BUNDLE_VERSION)
    {
        return static_cast<Common::SubdomainSelection>(s.value("settings/force_subdomain_selection", 0).toInt());
    }
    return Common::MC_DECIDE;
}

void MPDeviceBleImpl::handleLongMessageTimeout()
{
    qWarning() << "Timout for multiple packet expired";
    auto& cmd = mpDev->commandQueue.head();
    delete cmd.timerTimeout;
    cmd.timerTimeout = nullptr;
    cmd.running = false;
    cmd.checkReturn = true;
    cmd.response = QByteArray{};
    mpDev->sendDataDequeue();
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

QByteArray MPDeviceBleImpl::createChangePasswordMsg(const QByteArray& address, QString pwd)
{
    auto msg = address;
    msg.append(bleProt->toByteArray(pwd));
    msg.append(bleProt->toLittleEndianFromInt(static_cast<quint16>(0x0)));
    return msg;
}

QByteArray MPDeviceBleImpl::createUploadPasswordMessage(const QString &uploadPassword)
{
    if (uploadPassword.isEmpty())
    {
        return DEFAULT_BUNDLE_PASSWORD;
    }

    QByteArray passwordArray;
    if (uploadPassword.size() == UPLOAD_PASSWORD_BYTE_SIZE * 2)
    {
        const int BASE = 16;
        for (int i = 0; i < UPLOAD_PASSWORD_BYTE_SIZE; ++i)
        {
            bool ok = false;
            auto passwordChar = uploadPassword.mid(i*2, 2).toUInt(&ok, BASE);
            if (ok)
            {
                passwordArray.append(static_cast<char>(passwordChar));
            }
            else
            {
                qCritical() << "Invalid hex character";
                return QByteArray{};
            }
        }
    }
    else
    {
        qCritical() << "Invalid length of upload password";
        return QByteArray{};
    }

    return passwordArray;
}

void MPDeviceBleImpl::createAndAddCustomJob(QString name, std::function<void ()> fn)
{
    auto *jobs = new AsyncJobs(QString(name), this);
    auto *customJob = new CustomJob{this};
    customJob->setWork(std::move(fn));
    jobs->append(customJob);
    mpDev->jobsQueue.enqueue(jobs);
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
    if (!file.open(DeviceOpenModeFlag::ReadOnly))
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
    message.fill(ZERO_BYTE, BUNBLE_DATA_ADDRESS_SIZE);
    for (const auto byte : blob)
    {
        message.append(byte);
        ++byteCounter;
        if ((BUNBLE_DATA_WRITE_SIZE + BUNBLE_DATA_ADDRESS_SIZE) == byteCounter)
        {
            jobs->append(new MPCommandJob(mpDev, MPCmd::WRITE_256B_TO_FLASH, message,
                              [curAddress, cbProgress, fileSize](const QByteArray &data, bool &) -> bool
                                  {
                                      Q_UNUSED(data);
                                      QVariantMap progress = QVariantMap {
                                          {"total", fileSize},
                                          {"current", curAddress},
                                          {"msg", "Writing bundle data to device..." }
                                      };
                                      cbProgress(progress);

                                      if (AppDaemon::isDebugDev())
                                          qDebug() << "Sending message to address #" << curAddress;

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
    if (byteCounter != BUNBLE_DATA_ADDRESS_SIZE)
    {
        jobs->append(new MPCommandJob(mpDev, MPCmd::WRITE_256B_TO_FLASH, message,
                      [](const QByteArray &data, bool &) -> bool
                          {
                              Q_UNUSED(data);
                              qDebug() << "Sending bundle is DONE";
                              return true;
                          }));
    }

    jobs->append(new MPCommandJob(mpDev, MPCmd::END_BUNDLE_UPLOAD, bleProt->getDefaultFuncDone()));
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

bool MPDeviceBleImpl::isBundleFileReadable(const QString &filePath)
{
    return QFile{filePath}.open(DeviceOpenModeFlag::ReadOnly);
}
