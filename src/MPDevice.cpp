/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Moolticute is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Moolticute is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#include "MPDevice.h"
#include <functional>
#include "ParseDomain.h"
#include "MessageProtocolMini.h"
#include "MessageProtocolBLE.h"
#include "MPDeviceBleImpl.h"
#include "BleCommon.h"
#include "MPSettingsBLE.h"
#include "MPNodeBLE.h"
#include "AppDaemon.h"

MPDevice::MPDevice(QObject *parent):
    QObject(parent)
{
    set_status(Common::UnknownStatus);
    set_memMgmtMode(false); //by default device is not in MMM

    statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, [this]()
    {
        //Do not interfer with any other operation by sending a MOOLTIPASS_STATUS command
        if (commandQueue.size() > 0)
            return;

        sendData(MPCmd::MOOLTIPASS_STATUS, [this](bool success, const QByteArray &data, bool &)
        {
            if (!success)
                return;

            processStatusChange(data);
        });
    });

    connect(this, SIGNAL(platformDataRead(QByteArray)), this, SLOT(newDataRead(QByteArray)));

//    connect(this, SIGNAL(platformFailed()), this, SLOT(commandFailed()));
}

MPDevice::~MPDevice()
{
    filesCache.resetState();
    delete pMesProt;
    delete bleImpl;
}

void MPDevice::setupMessageProtocol()
{
    if (isBLE())
    {
        pMesProt = new MessageProtocolBLE{};
        bleImpl = new MPDeviceBleImpl(dynamic_cast<MessageProtocolBLE*>(pMesProt), this);
        pSettings = new MPSettingsBLE{this, pMesProt};
        qDebug() << "Mooltipass Mini BLE is connected";
    }
    else
    {
        pMesProt = new MessageProtocolMini{};
        pSettings = new MPSettingsMini{this, pMesProt};
        qDebug() << "Mooltipass Mini is connected";
    }

    importNodeMap = {
        {EXPORT_SERVICE_NODES_INDEX, &importedLoginNodes},
        {EXPORT_SERVICE_CHILD_NODES_INDEX, &importedLoginChildNodes},
        {EXPORT_WEBAUTHN_NODES_INDEX, &importedWebauthnLoginNodes},
        {EXPORT_WEBAUTHN_CHILD_NODES_INDEX, &importedWebauthnLoginChildNodes},
        {EXPORT_MC_SERVICE_NODES_INDEX, &importedDataNodes},
        {EXPORT_MC_SERVICE_CHILD_NODES_INDEX, &importedDataChildNodes},
    };


#ifndef Q_OS_WIN
    sendInitMessages();
#endif
}

void MPDevice::sendInitMessages()
{
    addTimerJob(INIT_STARTING_DELAY);

    if (isBLE())
    {
        /**
          * Reset Flip Bit is written directly to device
          * so TimerJob has no effect, hence sending
          * it with a lower timeout.
          */
        QTimer::singleShot(RESET_SEND_DELAY, this, &MPDevice::resetFlipBit);

        // For BLE no status timer, only sending first status request
        bleImpl->sendInitialStatusRequest();

        if (bleImpl->isAfterAuxFlash())
        {
            qDebug() << "Fixing communication with device after Aux Flash";
            writeCancelRequest();
        }
        bleImpl->getPlatInfo();
    }
    else
    {
        statusTimer->start(STATUS_STARTING_DELAY);
    }

    exitMemMgmtMode(false);
}

void MPDevice::sendData(MPCmd::Command c, const QByteArray &data, quint32 timeout, MPCommandCb cb, bool checkReturn)
{
    MPCommand cmd;

    // Prepare MP packet
    cmd.data = pMesProt->createPackets(data, c);
    cmd.cb = std::move(cb);
    cmd.checkReturn = checkReturn;
    cmd.retries_done = 0;
    cmd.sent_ts = QDateTime::currentMSecsSinceEpoch();

    if (!isBLE())
    {
        cmd.timerTimeout = new QTimer(this);
        connect(cmd.timerTimeout, &QTimer::timeout, [this]()
        {
            auto cmd = pMesProt->getCommand(commandQueue.head().data[0]);
            commandQueue.head().retry--;

            //Retry is disabled for BLE
            if (commandQueue.head().retry > 0)
            {
                qDebug() << "> Retry command: " << pMesProt->printCmd(cmd);
                commandQueue.head().sent_ts = QDateTime::currentMSecsSinceEpoch();
                commandQueue.head().timerTimeout->start(); //restart timer
                commandQueue.head().retries_done++;
                for (const auto &data : commandQueue.head().data)
                {
                    platformWrite(data);
                }
            }
            else
            {
                //Failed after all retry
                MPCommand currentCmd = commandQueue.head();
                delete currentCmd.timerTimeout;

                if (isBLE())
                {
                    qDebug() << "No response received from the device for: " << pMesProt->printCmd(cmd);
                }
                else
                {
                    qWarning() << "> Retry command: " << pMesProt->printCmd(cmd) << " has failed too many times. Give up.";
                }

                bool done = true;
                currentCmd.cb(false, QByteArray(3, 0x00), done);

                if (done)
                {
                    commandQueue.dequeue();
                    sendDataDequeue();
                }
            }
        });
        if (timeout == CMD_DEFAULT_TIMEOUT)
        {
            timeout = CMD_DEFAULT_TIMEOUT_VAL;

            //If user interaction is required, add additional timeout
            if (MPCmd::isUserRequired(c))
            {
                timeout += static_cast<quint32>(dynamic_cast<MPSettingsMini*>(pSettings)->get_user_interaction_timeout()) * 1000;
            }
        }
        cmd.timerTimeout->setInterval(static_cast<int>(timeout));
    }

    commandQueue.enqueue(cmd);

    if (!commandQueue.head().running)
        sendDataDequeue();
}

void MPDevice::sendData(MPCmd::Command cmd, quint32 timeout, MPCommandCb cb)
{
    sendData(cmd, QByteArray(), timeout, std::move(cb));
}

void MPDevice::sendData(MPCmd::Command cmd, MPCommandCb cb)
{
    sendData(cmd, QByteArray(), CMD_DEFAULT_TIMEOUT, std::move(cb));
}

void MPDevice::sendData(MPCmd::Command cmd, const QByteArray &data, MPCommandCb cb)
{
    sendData(cmd, data, CMD_DEFAULT_TIMEOUT, std::move(cb));
}

void MPDevice::enqueueAndRunJob(AsyncJobs *jobs)
{
    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::commandFailed()
{
    //TODO: fix this to work as it should on all platforms
    //this must be only called once when something went wrong
    //with the current command
//    MPCommand currentCmd = commandQueue.head();
//    currentCmd.cb(false, QByteArray());
//    commandQueue.dequeue();

//    QTimer::singleShot(150, this, SLOT(sendDataDequeue()));
}

void MPDevice::newDataRead(const QByteArray &data)
{
    //we assume that the QByteArray size is at least 64 bytes
    //this should be done by the platform code

    QByteArray dataReceived = data;
    bool isDebugStartMsg = false;
    bool isFirstBlePacket = isBLE() && bleImpl->isFirstPacket(data);
    if (isBLE())
    {
        isDebugStartMsg = isFirstBlePacket && pMesProt->getCommand(data) == MPCmd::DEBUG;
    }
    else
    {
        isDebugStartMsg = pMesProt->getCommand(data) == MPCmd::DEBUG;
    }
    if (m_isDebugMsg || isDebugStartMsg)
    {
        if (isBLE())
        {
            bleImpl->processDebugMsg(data, m_isDebugMsg);
        }
        else
        {
            qWarning() << data.toHex();
        }
        return;
    }

    if (commandQueue.isEmpty())
    {
        if (isBLE() && MPCmd::MOOLTIPASS_STATUS == pMesProt->getCommand(data))
        {
            qDebug() << "Received status: " << data.toHex();
            processStatusChange(data);
            return;
        }
        qWarning() << "Command queue is empty!";
        qWarning() << "Packet data " << " size:" << pMesProt->getMessageSize(data) << " data:" << data.toHex();
        return;
    }

    MPCommand currentCmd = commandQueue.head();
    const auto currentCommand = pMesProt->getCommand(currentCmd.data[0]);

    // First if: Resend the command, if device ask for retrying
    // Second if: Special case: if command check was requested but the device returned a mooltipass status (user entering his PIN), resend packet
    const auto dataCommand = pMesProt->getCommand(data);
    if ((dataCommand == MPCmd::PLEASE_RETRY && (isFirstBlePacket || !isBLE())) ||
        (currentCmd.checkReturn &&
        currentCommand != MPCmd::MOOLTIPASS_STATUS &&
        dataCommand == MPCmd::MOOLTIPASS_STATUS &&
        (pMesProt->getFirstPayloadByte(data) & MP_UNLOCKING_SCREEN_BITMASK) != 0))
    {
        if (!isBLE())
        {
            /* Stop timeout timer */
            commandQueue.head().timerTimeout->stop();
        }

        /* Bear with me for this complex explanation.
         * In some case, USB commands may take quite a while to get an answer, especially when the user is prompted (or is deliberately trying to delay the answer)
         * However, during long prompts, if a message is sent to the mini it will answer with a please retry or a status packet
         * In Moolticute, we have a timeout that will trigger message sending, so imagine the following sequence:
         * - MMM is asked
         * - user is playing with the prompt, delaying the answer
         * - timeout triggered, MC resends one packet
         * - the mooltipass replies with a please retry
         * In that case, there is twice the "go to MMM" packet "pending". So when we receive our please retry or status packet, we check that it is not for a message that actually was sent due to a timeout
         * And just to be sure, we checked that the mini was quick to answer the second message
         */
        if ((commandQueue.head().retries_done == 1) && ((QDateTime::currentMSecsSinceEpoch() - commandQueue.head().sent_ts) < 200))
        {
            qDebug() << pMesProt->printCmd(dataCommand) << " was received for a packet that was sent due to a timeout, not resending";
        }
        else
        {
            qDebug() << pMesProt->printCmd(dataCommand) << " received, resending command " << pMesProt->printCmd(currentCommand);
            QTimer *timer = new QTimer(this);
            connect(timer, &QTimer::timeout, [this, timer]()
            {
                timer->stop();
                timer->deleteLater();
                if (isBLE())
                {
                    sendDataDequeue();
                }
                else
                {
                    for (const auto &data : commandQueue.head().data)
                    {
                        platformWrite(data);
                    }
                    commandQueue.head().timerTimeout->start(); //restart timer
                }
            });
            timer->start(300);
        }
        return;
    }

    //Only check returned command if it was asked
    //If the returned command does not match, fail
    if (currentCmd.checkReturn &&
        dataCommand != currentCommand)
    {
        if (isBLE() && MPCmd::MOOLTIPASS_STATUS == dataCommand)
        {
            qDebug() << "Received status: " << data.toHex();
            processStatusChange(data);
            return;
        }
        qWarning() << "Wrong answer received: " << pMesProt->printCmd(dataCommand)
                   << " for command: " << pMesProt->printCmd(currentCommand);
        if (AppDaemon::isDebugDev())
            qWarning() << "Full response: " << data.toHex();

        return;
    }

    if (AppDaemon::isDebugDev())
    {
        QString resMsg = "Received answer: ";
        if (isBLE() && !bleImpl->isFirstPacket(data))
        {
            resMsg += pMesProt->printCmd(pMesProt->getCommand(currentCmd.data[0]));
        }
        else
        {
            qDebug() << "Message payload length:" << pMesProt->getMessageSize(data);
            resMsg += pMesProt->printCmd(dataCommand);
        }
        qDebug() << resMsg << " Full packet:" << data.toHex();
    }

    /**
      * For BLE it is waiting while every packet is received,
      * because the response has information about packet id
      * and full packet number.
      * For the first packet the entire packet is added to the
      * response QByteArray for backward compatibility reasons
      * with Mini and Classic.
      */
    if (isBLE())
    {
        if (!bleImpl->processReceivedData(data, dataReceived))
        {
            //Expecting more packet
            return;
        }
    }

    bool done = true;
    currentCmd.cb(true, dataReceived, done);
    delete currentCmd.timerTimeout;
    if (!commandQueue.empty())
    {
        commandQueue.head().timerTimeout = nullptr;
    }

    if (done)
    {
        if (!commandQueue.empty())
        {
            commandQueue.dequeue();
        }
        sendDataDequeue();
    }
    else
    {
        if (!commandQueue.empty())
        {
            commandQueue.head().checkReturn = false;
        }
    }
}

void MPDevice::sendDataDequeue()
{
    if (commandQueue.isEmpty())
        return;

    MPCommand &currentCmd = commandQueue.head();
    currentCmd.running = true;

    if (bleImpl && bleImpl->isNoBundle(pMesProt->getCommand(currentCmd.data[0])))
    {
        return;
    }

    int i = 0;
    if (AppDaemon::isDebugDev())
        qDebug() << "Platform send command: " << pMesProt->printCmd(currentCmd.data[0]);

    if (isBLE())
    {
        bleImpl->flipMessageBit(currentCmd.data);
        if (isBT() && !bleImpl->isFirstMessageWritten())
        {
            bleImpl->handleFirstBluetoothMessage(currentCmd);
        }
    }
    // send data with platform code
    for (const auto &data : currentCmd.data)
    {
        if (AppDaemon::isDebugDev())
        {
            auto toHex = [](quint16 b) -> QString { return QString("0x%1").arg((quint16)b, 2, 16, QChar('0')); };
            QString a = "[";
            for (int i = 0;i < data.size();i++)
            {
                a += toHex((quint8)data.at(i));
                if (i < data.size() - 1) a += ", ";
            }
            a += "]";

            qDebug() << "Full packet#" << i++ << ": " << a;
        }

        platformWrite(data);
    }

    if (isBLE())
    {
        /**
          * If checkReturn is false, not required to wait
          * for the response, so removing the cmd from
          * commandQueue and finishing currentJob.
          */
        if (!currentCmd.checkReturn)
        {
            currentJobs->finished(QByteArray{});
            commandQueue.dequeue();
        }
    }
    else
    {
        currentCmd.timerTimeout->start();
    }
}

void MPDevice::runAndDequeueJobs()
{
    if (jobsQueue.isEmpty() || currentJobs)
        return;

    currentJobs = jobsQueue.dequeue();

    connect(currentJobs, &AsyncJobs::finished, [this](const QByteArray &)
    {
        currentJobs = nullptr;
        runAndDequeueJobs();
    });
    connect(currentJobs, &AsyncJobs::failed, [this](AsyncJob *)
    {
        currentJobs = nullptr;
        runAndDequeueJobs();
    });

    currentJobs->start();
}

void MPDevice::resetFlipBit()
{
    if (AppDaemon::isDebugDev())
        qDebug() << "Resetting flip bit for BLE";
    bleImpl->sendResetFlipBit();
}

void MPDevice::addTimerJob(int msec)
{
    auto *waitingJob = new AsyncJobs(QString("Waiting job"), this);
    waitingJob->append(new TimerJob{msec});
    jobsQueue.enqueue(waitingJob);
    runAndDequeueJobs();
}

void MPDevice::updateFilesCache()
{
    getStoredFiles([this](bool success, QList<QVariantMap> files)
    {
        if (success)
        {
            filesCache.save(files);
            emit filesCacheChanged();
        }
    });
}

void MPDevice::addFileToCache(QString fileName, int size)
{
    QVariantMap item;
    item.insert("name", fileName);
    item.insert("size", size);

    // Add file name at proper position
    auto cache = filesCache.load();
    for (qint32 i = 0; i < cache.length(); i++)
    {
        if (cache[i].value("name").toString().compare(fileName) == 0)
        {
            // the file is already in cache, this is just and update
            return;
        }


        if (cache[i].value("name").toString().compare(fileName) > 0)
        {
            cache.insert(i, item);
            filesCache.save(cache);
            emit filesCacheChanged();
            return;
        }
    }

    cache.append(item);
    filesCache.save(cache);
    emit filesCacheChanged();
}

void MPDevice::updateFileInCache(QString fileName, int size)
{
    auto cache = filesCache.load();
    bool updated = false;
    for (int i = 0; i < cache.length() && !updated; i ++)
    {
        auto item = cache.at(i);
        if (item.value("name").toString().compare(fileName) == 0)
        {
            int revision = item.value("revision").toInt();
            item.insert("revision", revision  + 1);
            item.insert("size", size);

            cache.replace(i, item);
            updated = true;
        }
    }

    if (updated)
    {
        filesCache.save(cache);
        emit filesCacheChanged();
    }
}

void MPDevice::removeFileFromCache(QString fileName)
{
    auto cache = filesCache.load();
    int i = 0;
    for (; i < cache.length(); i++)
        if (cache.at(i).value("name").toString().compare(fileName) == 0)
            break;

    if (i < cache.length())
        cache.removeAt(i);

    filesCache.save(cache);
    emit filesCacheChanged();
}

void MPDevice::getBattery()
{
    if (isBLE())
    {
        bleImpl->getBattery();
    }
}

void MPDevice::resetCommunication()
{
    jobsQueue.clear();
    currentJobs = nullptr;
    commandQueue.clear();
}

void MPDevice::memMgmtModeReadFlash(AsyncJobs *jobs, bool fullScan,
                                    const MPDeviceProgressCb &cbProgress,bool getCreds,
                                    bool getData, bool getDataChilds)
{
    /* For when the MMM is left */
    cleanMMMVars();

    /* Get CTR value */
    jobs->append(new MPCommandJob(this, MPCmd::GET_CTRVALUE,
                                  [this, jobs](const QByteArray &data, bool &) -> bool
    {
        if (pMesProt->getMessageSize(data) == 1)
        {
            /* Received one byte as answer: command fail */
            jobs->setCurrentJobError("Mooltipass refused to send us a CTR packet");
            qCritical() << "Get CTR value: couldn't get answer";
            return false;
        }
        else
        {
            ctrValue = pMesProt->getFullPayload(data);
            ctrValueClone = pMesProt->getFullPayload(data);
            qDebug() << "CTR value:" << ctrValue.toHex();

            progressTotal = 100 + MOOLTIPASS_FAV_MAX;
            progressCurrent = 0;
            return true;
        }
    }));

    /* Get CPZ and CTR values */
    auto cpzJob = new MPCommandJob(this, MPCmd::GET_CARD_CPZ_CTR,
                                  [this, jobs](const QByteArray &data, bool &done) -> bool
    {
        const auto command = pMesProt->getCommand(data);
        /* The Mooltipass answers with CPZ CTR packets containing the CPZ_CTR values, and then a final MPCmd::GET_CARD_CPZ_CTR packet */
        if (command == MPCmd::CARD_CPZ_CTR_PACKET)
        {
            QByteArray cpz = pMesProt->getFullPayload(data);
            if(cpzCtrValue.contains(cpz))
            {
                qDebug() << "Duplicate CPZ CTR value:" << cpz.toHex();
            }
            else
            {
                qDebug() << "CPZ CTR value:" << cpz.toHex();
                cpzCtrValue.append(cpz);
                cpzCtrValueClone.append(cpz);
            }
            done = false;
            return true;
        }
        else if(command == MPCmd::GET_CARD_CPZ_CTR)
        {
            /* Received packet indicating we received all CPZ CTR packets */
            qDebug() << "All CPZ CTR packets received";
            if (isBLE())
            {
                const auto cpzCtr = pMesProt->getFullPayload(data);
                cpzCtrValue.append(cpzCtr);
                cpzCtrValueClone.append(cpzCtr);
            }
            return true;
        }
        else
        {
            qCritical() << "Get CPZ CTR: wrong command received as answer:" << pMesProt->printCmd(command);
            jobs->setCurrentJobError("Get CPZ/CTR: Mooltipass sent an answer packet with a different command ID");
            return false;
        }
    });
    if (!isBLE())
    {
        cpzJob->setReturnCheck(false); //disable return command check
    }
    jobs->append(cpzJob);

    /* Get favorites */
    if (isBLE())
    {
        jobs->append(new MPCommandJob(this, MPCmd::GET_FAVORITES,
                                  [this, jobs, cbProgress](const QByteArray &data, bool &)
                    {
                        if (pMesProt->getMessageSize(data) == 1)
                        {
                            /* Received one byte as answer: command fail */
                            jobs->setCurrentJobError("Mooltipass refused to send us favorites");
                            qCritical() << "Get favorite: couldn't get answer";
                            return false;
                        }
                        if (AppDaemon::isDebugDev())
                        {
                            qDebug() << "Received favorites: " << data.toHex();
                        }
                        /* Append favorite to list */
                        favoritesAddrs = bleImpl->getFavorites(data);
                        favoritesAddrsClone = favoritesAddrs;

                        progressCurrent++;
                        QVariantMap cbData = {
                            {"total", 1},
                            {"current", 1},
                            {"msg", "Favorite are loaded"}
                        };
                        cbProgress(cbData);
                        return true;
                    })
        );
    }
    else
    {
        for (uint i = 0; i < pMesProt->getMaxFavorite(); ++i)
        {
            jobs->append(new MPCommandJob(this, MPCmd::GET_FAVORITE,
                                          QByteArray(1, static_cast<quint8>(i)),
                                          [this, i, cbProgress](const QByteArray &, QByteArray &) -> bool
            {
                if (i == 0)
                {
                    qInfo() << "Loading favorites...";
                    QVariantMap data = {
                        {"total", progressTotal},
                        {"current", progressCurrent},
                        {"msg", "Loading Favorites..."}
                    };
                    cbProgress(data);
                }
                return true;
            },
                                          [this, i, jobs, cbProgress](const QByteArray &data, bool &) -> bool
            {
                if (pMesProt->getMessageSize(data) == 1)
                {
                    /* Received one byte as answer: command fail */
                    jobs->setCurrentJobError("Mooltipass refused to send us favorites");
                    qCritical() << "Get favorite: couldn't get answer";
                    return false;
                }
                else
                {
                    /* Append favorite to list */
                    qDebug() << "Favorite" << i << ": parent address:" << pMesProt->getPayloadBytes(data, 0, 2).toHex() << ", child address:" << pMesProt->getPayloadBytes(data, 2, 2).toHex();
                    favoritesAddrs.append(pMesProt->getPayloadBytes(data, 0, MOOLTIPASS_ADDRESS_SIZE));
                    favoritesAddrsClone.append(pMesProt->getPayloadBytes(data, 0, MOOLTIPASS_ADDRESS_SIZE));

                    progressCurrent++;
                    QVariantMap data = {
                        {"total", progressTotal},
                        {"current", progressCurrent},
                        {"msg", "Favorite %1 loaded"},
                        {"msg_args", QVariantList({i})}
                    };
                    cbProgress(data);

                    return true;
                }
            }));
        }
    }


    if (getCreds)
    {
        /* Get parent node start address */
        jobs->append(new MPCommandJob(this, MPCmd::GET_STARTING_PARENT,
                                      [this, jobs, fullScan, cbProgress](const QByteArray &data, bool &) -> bool
        {
            if (pMesProt->getMessageSize(data) == 1)
            {
                /* Received one byte as answer: command fail */
                jobs->setCurrentJobError("Mooltipass refused to send us starting parent");
                qCritical() << "Get start node addr: couldn't get answer";
                return false;
            }
            else
            {
                if (isBLE())
                {
                    startNode = bleImpl->processReceivedStartNodes(pMesProt->getFullPayload(data));
                    startNodeClone = startNode;
                }
                else
                {
                    startNode[Common::CRED_ADDR_IDX] = pMesProt->getFullPayload(data);
                    startNodeClone[Common::CRED_ADDR_IDX] = startNode[Common::CRED_ADDR_IDX];
                }
                qDebug() << "Start node addr:" << startNode[Common::CRED_ADDR_IDX].toHex();

                //if parent address is not null, load nodes
                if (startNode[Common::CRED_ADDR_IDX] != MPNode::EmptyAddress)
                {
                    qInfo() << "Loading parent nodes...";
                    if (!fullScan)
                    {
                        /* Traverse the flash by following the linked list */
                        loadLoginNode(jobs, startNode[Common::CRED_ADDR_IDX], cbProgress);
                    }
                    else
                    {
                        /* Full scan will be triggered once the answer from get data start node is received */
                    }
                }
                else
                {
                    qInfo() << "No parent nodes to load.";
                }

                if (isBLE())
                {
                    bleImpl->loadWebAuthnNodes(jobs, cbProgress);
                }

                return true;
            }
        }));
    }

    if (getData)
    {
        //Get parent data node start address
        jobs->append(new MPCommandJob(this, MPCmd::GET_DN_START_PARENT,
                                      [this, jobs, fullScan, getDataChilds, cbProgress](const QByteArray &data, bool &) -> bool
        {

            if (pMesProt->getMessageSize(data) == 1)
            {
                /* Received one byte as answer: command fail */
                jobs->setCurrentJobError("Mooltipass refused to send us data starting parent");
                qCritical() << "Get data start node addr: couldn't get answer";
                return false;
            }
            else
            {
                if (isBLE())
                {
                    startDataNode = bleImpl->getDataStartNode(pMesProt->getFullPayload(data));
                    startDataNodeClone = startDataNode;
                }
                else
                {
                    startDataNode = pMesProt->getFullPayload(data);
                    startDataNodeClone = pMesProt->getFullPayload(data);
                }
                qDebug() << "Start data node addr:" << startDataNode.toHex();

                //if data parent address is not null, load nodes
                if (startDataNode != MPNode::EmptyAddress)
                {
                    qInfo() << "Loading data parent nodes...";
                    if (!fullScan)
                    {
                        //full data nodes are not needed. Only parents for service name
                        loadDataNode(jobs, startDataNode, getDataChilds, cbProgress);
                    }
                }
                else
                {
                    /* Update files cache */
                    filesCache.save(QList<QVariantMap>());
                    filesCache.setDbChangeNumber(get_dataDbChangeNumber());
                    emit filesCacheChanged();
                    qInfo() << "No parent data nodes to load.";
                }

                //once we received start node address and data start node address, trigger full scan
                if (fullScan)
                {
                    /* Launch the scan */
                    loadSingleNodeAndScan(jobs, getMemoryFirstNodeAddress(), cbProgress);
                }

                return true;
            }
        }));
    }
}

void MPDevice::startMemMgmtMode(bool wantData,
                                const MPDeviceProgressCb &cbProgress,
                                const std::function<void(bool success, int errCode, QString errMsg)> &cb)
{
    /* Start MMM here, and load all memory data from the device */

    /* If we're already in MMM, return */
    if (get_memMgmtMode())
    {
        return;
    }

    /* New job for starting MMM */
    AsyncJobs *jobs = new AsyncJobs("Starting MMM mode", this);

    /* Ask device to go into MMM first */
    auto startMmmJob = new MPCommandJob(this, MPCmd::START_MEMORYMGMT, pMesProt->getDefaultFuncDone());
    startMmmJob->setTimeout(15000); //We need a big timeout here in case user enter a wrong pin code
    jobs->append(startMmmJob);

    /* Load flash contents the usual way */
    memMgmtModeReadFlash(jobs, false, cbProgress, !wantData, wantData, true);

    connect(jobs, &AsyncJobs::finished, [this, cb, wantData](const QByteArray &data)
    {
        Q_UNUSED(data);

        /* Tag favorites */
        if (!wantData)
        {
            tagFavoriteNodes();
        }

        /* Check DB */
        if (checkLoadedNodes(!wantData, wantData, false))
        {
            qInfo() << "Mem management mode enabled, DB checked";
            force_memMgmtMode(true);
            cb(true, 0, QString());
        }
        else
        {
            qInfo() << "DB has errors, leaving MMM";
            exitMemMgmtMode(true);

            //TODO: the errCode parameter could be used as an
            //identifier for a translated error message
            //The string is used for client (CLI for ex.) that
            //does not want to use the error code
            cb(false, 0, "Database Contains Errors, Please Run Integrity Check");
        }
    });

    connect(jobs, &AsyncJobs::failed, [this, cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Setting device in MMM failed";
        exitMemMgmtMode(true);

        //TODO: the errCode parameter could be used as an
        //identifier for a translated error message
        //The string is used for client (CLI for ex.) that
        //does not want to use the error code
        cb(false, 0, "Couldn't Load Database, Please Approve Prompt On Device");
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

QByteArray MPDevice::getMemoryFirstNodeAddress(void)
{
    /* Address format is 2 bytes little endian. last 3 bits are node number and first 13 bits are page address */
    switch(get_flashMbSize())
    {
        case(1): /* 128 pages for graphics */ return QByteArray::fromHex("0004");
        case(2): /* 128 pages for graphics */ return QByteArray::fromHex("0004");
        case(4): /* 256 pages for graphics */ return QByteArray::fromHex("0008");
        case(8): /* 256 pages for graphics */ return QByteArray::fromHex("0008");
        case(16): /* 256 pages for graphics */ return QByteArray::fromHex("0008");
        case(32): /* 128 pages for graphics */ return QByteArray::fromHex("0004");
        default: /* 256 pages for graphics */ return QByteArray::fromHex("0008");
    }
}

quint16 MPDevice::getNodesPerPage(void)
{
    if(get_flashMbSize() >= 16)
    {
        return 4;
    }
    else
    {
        return 2;
    }
}

quint16 MPDevice::getNumberOfPages(void)
{
    const auto flashSize = get_flashMbSize();
    if(flashSize >= 16)
    {
        return 256*flashSize;
    }
    else
    {
        return 512*flashSize;
    }
}

quint16 MPDevice::getFlashPageFromAddress(const QByteArray &address)
{
    return (((quint16)address[1] << 5) & 0x1FE0) | (((quint16)address[0] >> 3) & 0x001F);
}

QByteArray MPDevice::getNextNodeAddressInMemory(const QByteArray &address)
{
    /* Address format is 2 bytes little endian. last 3 bits are node number and first 13 bits are page address */
    quint8 curNodeInPage = address[0] & 0x07;
    quint16 curPage = getFlashPageFromAddress(address);

    // Do the incrementation
    if(++curNodeInPage == getNodesPerPage())
    {
        curNodeInPage = 0;
        curPage++;
    }

    // Reconstruct the address
    QByteArray return_data(address);
    return_data[0] = curNodeInPage | (((quint8)(curPage << 3)) & 0xF8);
    return_data[1] = (quint8)(curPage >> 5);
    return return_data;
}

void MPDevice::loadSingleNodeAndScan(AsyncJobs *jobs, const QByteArray &address, const MPDeviceProgressCb &cbProgress)
{
    /* Because of recursive calls, make sure we haven't reached the end of the memory */
    if (getFlashPageFromAddress(address) == getNumberOfPages())
    {
        qDebug() << "Reached the end of flash memory";
        return;
    }

    /* Progress bar */
    if (getFlashPageFromAddress(address) != lastFlashPageScanned)
    {
        lastFlashPageScanned = getFlashPageFromAddress(address);
        QVariantMap data = {
            {"total", getNumberOfPages()},
            {"current", lastFlashPageScanned},
            {"msg", "Scanning Mooltipass Memory (Read Speed: %1B/s)"},
            {"msg_args", QVariantList({diagLastNbBytesPSec})}
        };
        cbProgress(data);
    }

    /* For performance diagnostics */
    if (diagLastSecs != QDateTime::currentMSecsSinceEpoch()/1000)
    {
        qInfo() << "Current transfer speed:" << diagNbBytesRec << "B/s";
        diagLastSecs = QDateTime::currentMSecsSinceEpoch()/1000;
        diagLastNbBytesPSec = diagNbBytesRec;
        diagNbBytesRec = 0;
    }

    /* Create pointers to the nodes we are going to fill */
    MPNode *pnodeClone = pMesProt->createMPNode(this, address);
    MPNode *pnode = pMesProt->createMPNode(this, address);

    /* Send read node command, expecting 3 packets or 1 depending on if we're allowed to read a block*/
    jobs->append(new MPCommandJob(this, MPCmd::READ_FLASH_NODE,
                                  address,
                                  [this, jobs, pnodeClone, pnode, address, cbProgress](const QByteArray &data, bool &done) -> bool
    {
        if (pMesProt->getMessageSize(data) == 1)
        {
            diagTotalBlocks++;

            /* Received one byte as answer: we are not allowed to read */
            //qDebug() << "Loading Node" << getNodeIdFromAddress(address) << "at page" << getFlashPageFromAddress(address) << ": we are not allowed to read there";

            /* No point in keeping these nodes, simply delete them */
            delete pnodeClone;
            delete pnode;

            /* Load next node */
            loadSingleNodeAndScan(jobs, getNextNodeAddressInMemory(address), cbProgress);
            diagNbBytesRec += 64;
            return true;
        }
        else
        {
            /* Append received data to node data */
            const auto payload = pMesProt->getFullPayload(data);
            pnode->appendData(payload);
            pnodeClone->appendData(payload);

            // Continue to read data until the node is fully received
            if (!pnode->isDataLengthValid())
            {
                done = false;
            }
            else
            {
                diagTotalBlocks++;

                // Node is loaded
                if (!pnode->isValid())
                {
                    //qDebug() << address.toHex() << ": empty node loaded";
                    diagFreeBlocks++;

                    /* No point in keeping these nodes, simply delete them */
                    delete pnodeClone;
                    delete pnode;
                }
                else
                {
                    switch(pnode->getType())
                    {
                        case MPNode::NodeParent :
                        {
                            qDebug() << address.toHex() << ": parent node loaded:" << pnode->getService();
                            loginNodesClone.append(pnodeClone);
                            loginNodes.append(pnode);
                            break;
                        }
                        case MPNode::NodeChild :
                        {
                            qDebug() << address.toHex() << ": child node loaded:" << pnode->getLogin();
                            loginChildNodesClone.append(pnodeClone);
                            loginChildNodes.append(pnode);
                            break;
                        }
                        case MPNode::NodeParentData :
                        {
                            qDebug() << address.toHex() << ": data parent node loaded:" << pnode->getService() << "with start child addr:" << pnode->getStartChildAddress().toHex();
                            dataNodesClone.append(pnodeClone);
                            dataNodes.append(pnode);
                            break;
                        }
                        case MPNode::NodeChildData :
                        {
                            qDebug() << address.toHex() << ": data child node loaded";
                            dataChildNodesClone.append(pnodeClone);
                            dataChildNodes.append(pnode);
                            break;
                        }
                        default : break;
                    }
                }

                /* Load next node */
                loadSingleNodeAndScan(jobs, getNextNodeAddressInMemory(address), cbProgress);
                diagNbBytesRec += 64*3;
            }

            return true;
        }
    }));
}

void MPDevice::loadLoginNode(AsyncJobs *jobs, const QByteArray &address, const MPDeviceProgressCb &cbProgress, Common::AddressType addrType /*= Common::CRED_ADDR_IDX*/)
{
    qDebug() << "Loading cred parent node at address: " << address.toHex();

    /* Create new parent node, append to list */
    MPNode *pnode = pMesProt->createMPNode(this, address);
    MPNode *pnodeClone = pMesProt->createMPNode(this, address);
    if (isBLE())
    {
        bleImpl->appendLoginNode(pnode, pnodeClone, addrType);
    }
    else
    {
        loginNodes.append(pnode);
        loginNodesClone.append(pnodeClone);
    }

    /* Send read node command, expecting 3 packets */
    jobs->append(new MPCommandJob(this, MPCmd::READ_FLASH_NODE,
                                  address,
                                  [this, jobs, pnode, pnodeClone, address, cbProgress, addrType](const QByteArray &data, bool &done) -> bool
    {
        if (pMesProt->getMessageSize(data) == 1)
        {
            /* Received one byte as answer: command fail */
            jobs->setCurrentJobError("Couldn't read parent node, card removed or database corrupted");
            qCritical() << "Get node: couldn't get answer";
            return false;
        }
        else
        {
            /* Append received data to node data */
            const auto payload = pMesProt->getFullPayload(data);
            pnode->appendData(payload);
            pnodeClone->appendData(payload);
            QString srv = pnode->getService();

            //Continue to read data until the node is fully received
            if (!pnode->isDataLengthValid())
            {
                done = false;
            }
            else
            {
                if (srv.size() > 0)
                {
                    double currentFirstCharVal = srv.at(0).toLower().toLatin1();
                    if (currentFirstCharVal > 'z')
                        currentFirstCharVal = 'z';
                    if (currentFirstCharVal < 'a')
                        currentFirstCharVal = 'a';
                    progressCurrent = ((double)(currentFirstCharVal - 'a') / (double)('z' - 'a')) * 100;
                    progressCurrent += MOOLTIPASS_FAV_MAX;
                }

                //Node is loaded
                qDebug() << address.toHex() << ": parent node loaded:" << srv;

                QVariantMap data = {
                    {"total", progressTotal},
                    {"current", progressCurrent},
                    {"msg", "Loading credentials for %1" },
                    {"msg_args", QVariantList({srv})}
                };
                cbProgress(data);

                if (pnode->getStartChildAddress() != MPNode::EmptyAddress)
                {
                    qDebug() << srv << ": loading child nodes...";
                    loadLoginChildNode(jobs, pnode, pnodeClone, pnode->getStartChildAddress(), addrType);
                }
                else
                {
                    qDebug() << "Parent does not have childs.";
                }

                //Load next parent
                if (pnode->getNextParentAddress() != MPNode::EmptyAddress)
                {
                    loadLoginNode(jobs, pnode->getNextParentAddress(), cbProgress, addrType);
                }
            }

            return true;
        }
    }));
}

void MPDevice::loadLoginChildNode(AsyncJobs *jobs, MPNode *parent, MPNode *parentClone, const QByteArray &address, Common::AddressType addrType /*= Common::CRED_ADDR_IDX*/)
{
    qDebug() << "Loading cred child node at address:" << address.toHex();

    /* Create empty child node and add it to the list */
    MPNode *cnode = pMesProt->createMPNode(this, address);
    parent->appendChild(cnode);
    MPNode *cnodeClone = pMesProt->createMPNode(this, address);
    parentClone->appendChild(cnodeClone);
    if (isBLE())
    {
        bleImpl->appendLoginChildNode(cnode, cnodeClone, addrType);
    }
    else
    {
        loginChildNodes.append(cnode);
        loginChildNodesClone.append(cnodeClone);
    }

    /* Query node */
    jobs->prepend(new MPCommandJob(this, MPCmd::READ_FLASH_NODE,
                                  address,
                                  [this, jobs, cnode, cnodeClone, address, parent, parentClone, addrType](const QByteArray &data, bool &done) -> bool
    {
        if (pMesProt->getMessageSize(data) == 1)
        {
            /* Received one byte as answer: command fail */
            jobs->setCurrentJobError("Couldn't read child node, card removed or database corrupted");
            qCritical() << "Get child node: couldn't get answer";
            return false;
        }
        else
        {
            /* Append received data to node data */
            const auto payload = pMesProt->getFullPayload(data);
            cnode->appendData(payload);
            cnodeClone->appendData(payload);

            //Continue to read data until the node is fully received
            if (!cnode->isDataLengthValid())
            {
                done = false;
            }
            else
            {
                //Node is loaded
                qDebug() << address.toHex() << ": child node loaded:" << cnode->getLogin();

                //Load next child
                if (cnode->getNextChildAddress() != MPNode::EmptyAddress)
                {
                    loadLoginChildNode(jobs, parent, parentClone, cnode->getNextChildAddress(), addrType);
                }
            }

            return true;
        }
    }));
}

void MPDevice::loadDataNode(AsyncJobs *jobs, const QByteArray &address, bool load_childs, const MPDeviceProgressCb &cbProgress)
{
    MPNode *pnode = pMesProt->createMPNode(this, address);
    dataNodes.append(pnode);
    MPNode *pnodeClone = pMesProt->createMPNode(this, address);
    dataNodesClone.append(pnodeClone);

    qDebug() << "Loading data parent node at address: " << address.toHex();

    jobs->append(new MPCommandJob(this, MPCmd::READ_FLASH_NODE,
                                  address,
                                  [this, jobs, pnode, pnodeClone, load_childs, cbProgress](const QByteArray &data, bool &done) -> bool
    {
        if (pMesProt->getMessageSize(data) == 1)
        {
            /* Received one byte as answer: command fail */
            jobs->setCurrentJobError("Couldn't read data node, card removed or database corrupted");
            qCritical() << "Get data node: couldn't get answer";
            return false;
        }

        const auto payload = pMesProt->getFullPayload(data);
        pnode->appendData(payload);
        pnodeClone->appendData(payload);

        //Continue to read data until the node is fully received
        if (!pnode->isValid())
        {
            done = false;
        }
        else
        {
            QVariantMap data = {
                {"total", -1},
                {"current", 0},
                {"msg", "Loading data for %1" },
                {"msg_args", QVariantList({pnode->getService()})}
            };
            cbProgress(data);

            //Node is loaded
            qDebug() << "Parent data node loaded: " << pnode->getService() << " at address " << pnode->getAddress().toHex() << " first child at " << pnode->getStartChildAddress().toHex();

            //Load data child
            if (pnode->getStartChildAddress() != MPNode::EmptyAddress && load_childs)
            {
                qDebug() << "Loading data child nodes...";
                loadDataChildNode(jobs, pnode, pnodeClone, pnode->getStartChildAddress(), cbProgress, 0);
            }
            else
            {
                if (pnode->getStartChildAddress() == MPNode::EmptyAddress)
                {
                    qDebug() << "Parent data node does not have childs.";
                }
            }

            //Load next parent
            if (pnode->getNextParentAddress() != MPNode::EmptyAddress)
            {
                loadDataNode(jobs, pnode->getNextParentAddress(), load_childs, cbProgress);
            }
        }

        return true;
    }));
}

void MPDevice::loadDataChildNode(AsyncJobs *jobs, MPNode *parent, MPNode *parentClone, const QByteArray &address, const MPDeviceProgressCb &cbProgress, quint32 nbBytesFetched)
{
    MPNode *cnode = pMesProt->createMPNode(this, address);
    parent->appendChildData(cnode);
    dataChildNodes.append(cnode);
    MPNode *cnodeClone = pMesProt->createMPNode(this, address);
    parentClone->appendChildData(cnodeClone);
    dataChildNodesClone.append(cnodeClone);

    qDebug() << "Loading data child node at address: " << address.toHex();

    jobs->prepend(new MPCommandJob(this, MPCmd::READ_FLASH_NODE,
                                  address,
                                  [this, jobs, cnode, cnodeClone, cbProgress, nbBytesFetched, parent, parentClone](const QByteArray &data, bool &done) -> bool
    {
        if (pMesProt->getMessageSize(data) == 1)
        {
            /* Received one byte as answer: command fail */
            jobs->setCurrentJobError("Couldn't read data child node, card removed or database corrupted");
            qCritical() << "Get data child node: couldn't get answer";
            return false;
        }

        const auto payload = pMesProt->getFullPayload(data);
        cnode->appendData(payload);
        cnodeClone->appendData(payload);

        //Continue to read data until the node is fully received
        if (!cnode->isValid())
        {
            done = false;
        }
        else
        {
            //Node is loaded
            qDebug() << "Child data node loaded";
            const auto dataEncSize = pMesProt->getDataNodeEncSize();

            QVariantMap data = {
                {"total", -1},
                {"current", 0},
                {"msg", "Loading data for %1: %2 encrypted bytes read" },
                {"msg_args", QVariantList({parent->getService(), nbBytesFetched + dataEncSize})}
            };
            cbProgress(data);

            //Load next child
            if (cnode->getNextChildDataAddress() != MPNode::EmptyAddress)
            {
                loadDataChildNode(jobs, parent, parentClone, cnode->getNextChildDataAddress(), cbProgress, nbBytesFetched + dataEncSize);
            }
            else
            {
                parent->setEncDataSize(nbBytesFetched + dataEncSize);
                parentClone->setEncDataSize(nbBytesFetched + dataEncSize);

                /* and if our parent doesn't have next one... */
                if (parent->getNextParentAddress() == MPNode::EmptyAddress)
                {
                    // No next parent, fill our file cache
                    QList<QVariantMap> list;
                    for (auto &i: dataNodes)
                    {
                        QVariantMap item;
                        item.insert("revision", 0);
                        item.insert("name", i->getService());
                        item.insert("size", i->getEncDataSize());
                        list.append(item);
                    }
                    filesCache.save(list);
                    filesCache.setDbChangeNumber(get_dataDbChangeNumber());
                    emit filesCacheChanged();
                }
            }
        }

        return true;
    }));
}

/* Find a credential parent node given a child address */
MPNode* MPDevice::findCredParentNodeGivenChildNodeAddr(const QByteArray &address, const quint32 virt_addr)
{
    QListIterator<MPNode*> i(loginNodes);
    while (i.hasNext())
    {
        MPNode* nodeItem = i.next();

        /* Get first child node addr */
        QByteArray curChildNodeAddr = nodeItem->getStartChildAddress();
        quint32 curChildNodeAddr_v = nodeItem->getStartChildVirtualAddress();

        /* Check every children */
        while ((curChildNodeAddr != MPNode::EmptyAddress) || (curChildNodeAddr.isNull() && curChildNodeAddr_v != 0))
        {
            MPNode* curNode = findNodeWithAddressInList(loginChildNodes, curChildNodeAddr, curChildNodeAddr_v);

            /* Safety checks */
            if (!curNode)
            {
                qCritical() << "Couldn't find child node in list (corrupted DB?)";
                return nullptr;
            }

            /* Correct one? */
            if ((!curChildNodeAddr.isNull() && curChildNodeAddr == address) || (curChildNodeAddr.isNull() && curChildNodeAddr_v == virt_addr))
            {
                return nodeItem;
            }

            /* Next item */
            curChildNodeAddr = curNode->getNextChildAddress();
            curChildNodeAddr_v = curNode->getNextChildVirtualAddress();
        }
    }

    return nullptr;
}

void MPDevice::deletePossibleFavorite(QByteArray parentAddr, QByteArray childAddr)
{
    QByteArray curAddr = QByteArray();
    curAddr.append(parentAddr);
    curAddr.append(childAddr);

    for (qint32 i = 0; i < favoritesAddrs.size(); i++)
    {
        if (favoritesAddrs[i] == curAddr)
        {
            favoritesAddrs[i] = QByteArray(4, 0);
        }
    }
}

/* Find a node inside a given list given his address */
MPNode *MPDevice::findNodeWithAddressInList(NodeList list, const QByteArray &address, const quint32 virt_addr)
{
    auto it = std::find_if(list.begin(), list.end(), [&address, virt_addr](const MPNode *const node)
    {
        if (node->getAddress().isNull())
        {
            return node->getVirtualAddress() == virt_addr;
        }
        else
        {
            return node->getAddress() == address;
        }
    });

    return it == list.end()?nullptr:*it;
}

/* Find a node inside a given list given his address */
MPNode *MPDevice::findNodeWithNameInList(NodeList list, const QString& name, bool isParent)
{
    auto it = std::find_if(list.begin(), list.end(), [&name, isParent](const MPNode *const node)
    {
        if (isParent)
        {
            return node->getService() == name;
        }
        else
        {
            return node->getLogin() == name;
        }
    });

    return it == list.end()?nullptr:*it;
}

/* Find a node inside a given list given his address */
MPNode *MPDevice::findNodeWithLoginWithGivenParentInList(NodeList list,  MPNode *parent, const QString& name)
{
    /* get first child */
    MPNode* tempChildNodePt;
    QByteArray tempChildAddress = parent->getStartChildAddress();
    quint32 tempVirtualChildAddress = parent->getStartChildVirtualAddress();

    /* browse through all the children */
    while ((tempChildAddress != MPNode::EmptyAddress) || (tempChildAddress.isNull() && tempVirtualChildAddress != 0))
    {
        /* Get pointer to the child node */
        tempChildNodePt = findNodeWithAddressInList(list, tempChildAddress, tempVirtualChildAddress);

        /* Check we could find child pointer */
        if (!tempChildNodePt)
        {
            qWarning() << "findNodeWithLoginWithGivenParentInList: couldn't find child node with address" << tempChildAddress.toHex() << "in our list";
            return nullptr;
        }
        else
        {
            /* Check if it has the right name */
            if (tempChildNodePt->getLogin() == name)
            {
                return tempChildNodePt;
            }

            /* Loop to next possible child */
            tempChildAddress = tempChildNodePt->getNextChildAddress();
            tempVirtualChildAddress = tempChildNodePt->getNextChildVirtualAddress();
        }
    }

    /* Went through the list of children, didn't find anything */
    return nullptr;
}


/* Find a node inside a given list given his address */
MPNode *MPDevice::findNodeWithAddressWithGivenParentInList(NodeList list,  MPNode *parent, const QByteArray &address, const quint32 virt_addr)
{
    /* get first child */
    MPNode* tempChildNodePt;
    QByteArray tempChildAddress = parent->getStartChildAddress();
    quint32 tempVirtualChildAddress = parent->getStartChildVirtualAddress();

    /* browse through all the children */
    while ((tempChildAddress != MPNode::EmptyAddress) || (tempChildAddress.isNull() && tempVirtualChildAddress != 0))
    {
        /* Get pointer to the child node */
        tempChildNodePt = findNodeWithAddressInList(list, tempChildAddress, tempVirtualChildAddress);

        /* Check we could find child pointer */
        if (!tempChildNodePt)
        {
            qWarning() << "findNodeWithLoginWithGivenParentInList: couldn't find child node with address" << tempChildAddress.toHex() << "in our list";
            return nullptr;
        }
        else
        {
            /* Check if it has the right address */
            if (tempChildNodePt->getAddress().isNull())
            {
                if (tempChildNodePt->getVirtualAddress() == virt_addr)
                {
                    return tempChildNodePt;
                }
            }
            else
            {
                if (tempChildNodePt->getAddress() == address)
                {
                    return tempChildNodePt;
                }
            }

            /* Loop to next possible child */
            tempChildAddress = tempChildNodePt->getNextChildAddress();
            tempVirtualChildAddress = tempChildNodePt->getNextChildVirtualAddress();
        }
    }

    /* Went through the list of children, didn't find anything */
    return nullptr;
}

/* Find a node inside the parent list given his service */
MPNode *MPDevice::findNodeWithServiceInList(const QString &service, Common::AddressType addrType /*= Common::CRED_ADDR_IDX*/)
{
    NodeList& nodes = Common::CRED_ADDR_IDX == addrType ? loginNodes : webAuthnLoginNodes;
    auto it = std::find_if(nodes.begin(), nodes.end(), [&service](const MPNode *const node)
    {
        return node->getService() == service;
    });

    return it == nodes.end()?nullptr:*it;
}

bool MPDevice::tagFavoriteNodes(void)
{
    quint32 tempVirtualParentAddress;
    QByteArray tempParentAddress;
    quint32 tempVirtualChildAddress;
    QByteArray tempChildAddress;
    MPNode* tempParentNodePt = nullptr;
    MPNode* tempChildNodePt = nullptr;

    /* start with start node (duh) */
    tempParentAddress = startNode[Common::CRED_ADDR_IDX];
    tempVirtualParentAddress = virtualStartNode[Common::CRED_ADDR_IDX];

    /* Loop through the parent nodes */
    while ((tempParentAddress != MPNode::EmptyAddress) || (tempParentAddress.isNull() && tempVirtualParentAddress != 0))
    {
        /* Get pointer to next parent node */
        tempParentNodePt = findNodeWithAddressInList(loginNodes, tempParentAddress, tempVirtualParentAddress);

        /* Check that we could actually find it */
        if (!tempParentNodePt)
        {
            qCritical() << "tagFavoriteNodes: couldn't find parent node with address" << tempParentAddress.toHex() << "in our list";
            return false;
        }
        else
        {
            /* get first child */
            tempChildAddress = tempParentNodePt->getStartChildAddress();
            tempVirtualChildAddress = tempParentNodePt->getStartChildVirtualAddress();

            /* browse through all the children */
            while ((tempChildAddress != MPNode::EmptyAddress) || (tempChildAddress.isNull() && tempVirtualChildAddress != 0))
            {
                /* Get pointer to the child node */
                tempChildNodePt = findNodeWithAddressInList(loginChildNodes, tempChildAddress, tempVirtualChildAddress);

                /* Check we could find child pointer */
                if (!tempChildNodePt)
                {
                    qWarning() << "tagFavoriteNodes: couldn't find child node with address" << tempChildAddress.toHex() << "in our list";
                    return false;
                }
                else
                {
                    /* Check if it is a favorite */
                    if (tempParentAddress != MPNode::EmptyAddress && !tempParentAddress.isNull() && tempChildAddress != MPNode::EmptyAddress && !tempChildAddress.isNull())
                    {
                        QByteArray curParentChildFavAddr = QByteArray();
                        curParentChildFavAddr.append(tempParentAddress);
                        curParentChildFavAddr.append(tempChildAddress);

                        for (qint32 i = 0; i < favoritesAddrs.size(); i++)
                        {
                            if (favoritesAddrs[i] == curParentChildFavAddr)
                            {
                                qDebug() << tempChildNodePt->getLogin() << " is favorite #" << i;
                                tempChildNodePt->setFavoriteProperty((quint8)i);
                            }
                        }
                    }

                    /* Loop to next possible child */
                    tempChildAddress = tempChildNodePt->getNextChildAddress();
                    tempVirtualChildAddress = tempChildNodePt->getNextChildVirtualAddress();
                }
            }

            /* get next parent address */
            tempParentAddress = tempParentNodePt->getNextParentAddress();
            tempVirtualParentAddress = tempParentNodePt->getNextParentVirtualAddress();
        }
    }

    return true;
}

void MPDevice::detagPointedNodes(Common::AddressType addrType /*= Common::CRED_ADDR_IDX*/)
{
    if (Common::CRED_ADDR_IDX == addrType)
    {
        for (auto &i: loginNodes)
        {
            i->removePointedToCheck();
        }
        for (auto &i: loginChildNodes)
        {
            i->removePointedToCheck();
        }
        for (auto &i: dataNodes)
        {
            i->removePointedToCheck();
        }
        for (auto &i: dataChildNodes)
        {
            i->removePointedToCheck();
        }
    }
    else
    {
        for (auto &i: webAuthnLoginNodes)
        {
            i->removePointedToCheck();
        }
        for (auto &i: webAuthnLoginChildNodes)
        {
            i->removePointedToCheck();
        }
    }
}

/* Follow the chain to tag pointed nodes (useful when doing integrity check when we are getting everything we can) */
bool MPDevice::tagPointedNodes(bool tagCredentials, bool tagData, bool repairAllowed, Common::AddressType addrType /*= Common::CRED_ADDR_IDX*/)
{
    quint32 tempVirtualParentAddress;
    QByteArray tempParentAddress;
    quint32 tempVirtualChildAddress;
    QByteArray tempChildAddress;
    MPNode* tempNextParentNodePt = nullptr;
    MPNode* tempParentNodePt = nullptr;
    MPNode* tempNextChildNodePt = nullptr;
    MPNode* tempChildNodePt = nullptr;
    bool return_bool = true;
    const bool isCred = Common::CRED_ADDR_IDX == addrType;
    NodeList& nodes = isCred ? loginNodes : webAuthnLoginNodes;
    NodeList& childNodes = isCred? loginChildNodes : webAuthnLoginChildNodes;

    /* first, detag all nodes */
    detagPointedNodes(addrType);

    if (tagCredentials)
    {
        /* start with start node (duh) */
        tempParentAddress = startNode[addrType];
        tempVirtualParentAddress = virtualStartNode[addrType];

        /* Loop through the parent nodes */
        while ((tempParentAddress != MPNode::EmptyAddress) || (tempParentAddress.isNull() && tempVirtualParentAddress != 0))
        {
            /* Get pointer to next parent node */
            tempNextParentNodePt = findNodeWithAddressInList(nodes, tempParentAddress, tempVirtualParentAddress);

            /* Check that we could actually find it */
            if (!tempNextParentNodePt)
            {
                qCritical() << "tagPointedNodes: couldn't find parent node with address" << tempParentAddress.toHex() << "in our list";

                if (repairAllowed)
                {
                    if ((!tempParentAddress.isNull() && tempParentAddress == startNode[addrType]) || (tempParentAddress.isNull() && tempVirtualParentAddress == virtualStartNode[addrType]))
                    {
                        /* start node is incorrect */
                        startNode[addrType] = QByteArray(MPNode::EmptyAddress);
                        virtualStartNode[addrType] = 0;
                    }
                    else
                    {
                        /* previous parent next address is incorrect */
                        tempParentNodePt->setNextParentAddress(MPNode::EmptyAddress);
                    }
                }

                /* Stop looping */
                return_bool = false;
                break;
            }
            else if (tempNextParentNodePt->getPointedToCheck())
            {
                /* Linked chain loop detected */
                qCritical() << "tagPointedNodes: parent node loop has been detected: parent node with address" << tempParentNodePt->getAddress().toHex() << " points to parent node with address" << tempParentAddress.toHex();

                if (repairAllowed)
                {
                    if ((!tempParentAddress.isNull() && tempParentAddress == startNode[addrType]) || (tempParentAddress.isNull() && tempVirtualParentAddress == virtualStartNode[addrType]))
                    {
                        /* start node is already tagged... how's that possible? */
                        startNode[addrType] = QByteArray(MPNode::EmptyAddress);
                        virtualStartNode[addrType] = 0;
                    }
                    else
                    {
                        /* previous parent next address is incorrect */
                        tempParentNodePt->setNextParentAddress(MPNode::EmptyAddress);
                    }
                }

                /* Stop looping */
                return_bool = false;
                break;
            }
            else
            {
                /* check previous node address */
                if ((!tempParentAddress.isNull() && tempParentAddress == startNode[addrType]) || (tempParentAddress.isNull() && tempVirtualParentAddress == virtualStartNode[addrType]))
                {
                    /* first parent node: previous address should be an empty one */
                    if ((tempNextParentNodePt->getPreviousParentAddress() != MPNode::EmptyAddress) || (tempNextParentNodePt->getPreviousParentAddress().isNull() && tempNextParentNodePt->getPreviousParentVirtualAddress() != 0))
                    {
                        qWarning() << "tagPointedNodes: parent node" << tempNextParentNodePt->getService() <<  "at address" << tempParentAddress.toHex() << "has incorrect previous address:" << tempNextParentNodePt->getPreviousParentAddress().toHex() << "instead of" << MPNode::EmptyAddress.toHex();
                        if (repairAllowed)
                        {
                            tempNextParentNodePt->setPreviousParentAddress(MPNode::EmptyAddress);
                        }
                        return_bool = false;
                    }
                }
                else
                {
                    /* normal linked chain */
                    if ((!tempNextParentNodePt->getPreviousParentAddress().isNull() && tempNextParentNodePt->getPreviousParentAddress() != tempParentNodePt->getAddress()) || (tempNextParentNodePt->getPreviousParentAddress().isNull() && tempNextParentNodePt->getPreviousParentVirtualAddress() != tempParentNodePt->getVirtualAddress()))
                    {
                        qWarning() << "tagPointedNodes: parent node" << tempNextParentNodePt->getService() <<  "at address" << tempParentAddress.toHex() << "has incorrect previous address:" << tempNextParentNodePt->getPreviousParentAddress().toHex() << "instead of" << tempParentNodePt->getAddress().toHex();
                        if (repairAllowed)
                        {
                            tempNextParentNodePt->setPreviousParentAddress(tempParentNodePt->getAddress(), tempParentNodePt->getVirtualAddress());
                        }
                        return_bool = false;
                    }
                }

                /* Set correct pointed node */
                tempParentNodePt = tempNextParentNodePt;

                /* tag parent */
                tempParentNodePt->setPointedToCheck();

                /* get first child */
                tempChildAddress = tempParentNodePt->getStartChildAddress();
                tempVirtualChildAddress = tempParentNodePt->getStartChildVirtualAddress();

                /* browse through all the children */
                while ((tempChildAddress != MPNode::EmptyAddress) || (tempChildAddress.isNull() && tempVirtualChildAddress != 0))
                {
                    /* Get pointer to the child node */
                    tempNextChildNodePt = findNodeWithAddressInList(childNodes, tempChildAddress, tempVirtualChildAddress);

                    /* Check we could find child pointer */
                    if (!tempNextChildNodePt)
                    {
                        qWarning() << "tagPointedNodes: couldn't find child node with address" << tempChildAddress.toHex() << "in our list";
                        return_bool = false;

                        if (repairAllowed)
                        {
                            if ((!tempChildAddress.isNull() && tempChildAddress == tempParentNodePt->getStartChildAddress()) || (tempChildAddress.isNull() && tempVirtualChildAddress == tempParentNodePt->getStartChildVirtualAddress()))
                            {
                                // first child
                                tempParentNodePt->setStartChildAddress(MPNode::EmptyAddress);
                            }
                            else
                            {
                                // fix prev child
                                tempChildNodePt->setNextChildAddress(MPNode::EmptyAddress);
                            }
                        }

                        /* Loop to next parent */
                        tempChildAddress = MPNode::EmptyAddress;
                    }
                    else if (tempNextChildNodePt->getPointedToCheck())
                    {
                        /* Linked chain loop detected */
                        if ((!tempChildAddress.isNull() && tempChildAddress == tempParentNodePt->getStartChildAddress()) || (tempChildAddress.isNull() && tempVirtualChildAddress == tempParentNodePt->getStartChildVirtualAddress()))
                        {
                            qCritical() << "tagPointedNodes: child node already pointed to: parent node with address" << tempParentAddress.toHex() << " points to child node with address" << tempChildAddress.toHex();
                            if (repairAllowed)
                            {
                                tempParentNodePt->setStartChildAddress(MPNode::EmptyAddress);
                            }
                        }
                        else
                        {
                            qCritical() << "tagPointedNodes: child node loop has been detected: child node with address" << tempChildNodePt->getAddress().toHex() << " points to child node with address" << tempChildAddress.toHex();
                            if (repairAllowed)
                            {
                                tempChildNodePt->setNextChildAddress(MPNode::EmptyAddress);
                            }
                        }

                        /* Stop looping */
                        return_bool = false;
                        break;
                    }
                    else
                    {
                        /* check previous node address */
                        if ((!tempChildAddress.isNull() && tempChildAddress == tempParentNodePt->getStartChildAddress()) || (tempChildAddress.isNull() && tempVirtualChildAddress == tempParentNodePt->getStartChildVirtualAddress()))
                        {
                            /* first child node in given parent: previous address should be an empty one */
                            if ((!tempNextChildNodePt->getPreviousChildAddress().isNull() && tempNextChildNodePt->getPreviousChildAddress() != MPNode::EmptyAddress) || (tempNextChildNodePt->getPreviousChildAddress().isNull() && tempNextChildNodePt->getPreviousChildVirtualAddress() != 0))
                            {
                                qWarning() << "tagPointedNodes: child node" << tempNextChildNodePt->getLogin() <<  "at address" << tempChildAddress.toHex() << "has incorrect previous address:" << tempNextChildNodePt->getPreviousChildAddress().toHex() << "instead of" << MPNode::EmptyAddress.toHex();
                                if (repairAllowed)
                                {
                                    tempNextChildNodePt->setPreviousChildAddress(MPNode::EmptyAddress);
                                }
                                return_bool = false;
                            }
                        }
                        else
                        {
                            /* normal linked chain */
                            if ((!tempNextChildNodePt->getPreviousChildAddress().isNull() && tempNextChildNodePt->getPreviousChildAddress() != tempChildNodePt->getAddress()) || (tempNextChildNodePt->getPreviousChildAddress().isNull() && tempNextChildNodePt->getPreviousChildVirtualAddress() != tempChildNodePt->getVirtualAddress()))
                            {
                                qWarning() << "tagPointedNodes: child node" << tempNextChildNodePt->getLogin() <<  "at address" << tempChildAddress.toHex() << "has incorrect previous address:" << tempNextChildNodePt->getPreviousChildAddress().toHex() << "instead of" << tempChildNodePt->getAddress().toHex();
                                if (repairAllowed)
                                {
                                    tempNextChildNodePt->setPreviousChildAddress(tempChildNodePt->getAddress());
                                }
                                return_bool = false;
                            }
                        }

                        /* Set correct pointed node */
                        tempChildNodePt = tempNextChildNodePt;

                        /* Tag child */
                        tempChildNodePt->setPointedToCheck();

                        /* Loop to next possible child */
                        tempChildAddress = tempChildNodePt->getNextChildAddress();
                        tempVirtualChildAddress = tempChildNodePt->getNextChildVirtualAddress();
                    }
                }

                /* get next parent address */
                tempParentAddress = tempParentNodePt->getNextParentAddress();
                tempVirtualParentAddress = tempParentNodePt->getNextParentVirtualAddress();
            }
        }
    }

    if (tagData && isCred)
    {
        /** SAME FOR DATA NODES **/
        /* start with start node (duh) */
        tempParentAddress = startDataNode;
        tempVirtualParentAddress = virtualDataStartNode;

        /* Loop through the parent nodes */
        while ((tempParentAddress != MPNode::EmptyAddress) || (tempParentAddress.isNull() && tempVirtualParentAddress != 0))
        {
            /* Get pointer to next parent node */
            tempNextParentNodePt = findNodeWithAddressInList(dataNodes, tempParentAddress, tempVirtualParentAddress);

            /* Check that we could actually find it */
            if (!tempNextParentNodePt)
            {
                qCritical() << "tagPointedNodes: couldn't find data parent node with address" << tempParentAddress.toHex() << "in our list";

                if (repairAllowed)
                {
                    if ((!tempParentAddress.isNull() && tempParentAddress == startDataNode) || (tempParentAddress.isNull() && tempVirtualParentAddress == virtualDataStartNode))
                    {
                        /* start node is incorrect */
                        startDataNode = QByteArray(MPNode::EmptyAddress);
                        virtualDataStartNode = 0;
                    }
                    else
                    {
                        /* previous parent next address is incorrect */
                        tempParentNodePt->setNextParentAddress(MPNode::EmptyAddress);
                    }
                }

                /* Stop looping */
                return_bool = false;
                break;
            }
            else if (tempNextParentNodePt->getPointedToCheck())
            {
                /* Linked chain loop detected */
                qCritical() << "tagPointedNodes: data parent node loop has been detected: parent node with address" << tempParentNodePt->getAddress().toHex() << " points to parent node with address" << tempParentAddress.toHex();

                if (repairAllowed)
                {
                    if ((!tempParentAddress.isNull() && tempParentAddress == startDataNode) || (tempParentAddress.isNull() && tempVirtualParentAddress == virtualDataStartNode))
                    {
                        /* start node is already tagged... how's that even possible? */
                        startDataNode = QByteArray(MPNode::EmptyAddress);
                        virtualDataStartNode = 0;
                    }
                    else
                    {
                        /* previous parent next address is incorrect */
                        tempParentNodePt->setNextParentAddress(MPNode::EmptyAddress);
                    }
                }

                /* Stop looping */
                return_bool = false;
                break;
            }
            else
            {
                /* check previous node address */
                if ((!tempParentAddress.isNull() && tempParentAddress == startDataNode) || (tempParentAddress.isNull() && tempVirtualParentAddress == virtualDataStartNode))
                {
                    /* first parent node: previous address should be an empty one */
                    if ((tempNextParentNodePt->getPreviousParentAddress() != MPNode::EmptyAddress) || (tempNextParentNodePt->getPreviousParentAddress().isNull() && tempNextParentNodePt->getPreviousParentVirtualAddress() != 0))
                    {
                        qWarning() << "tagPointedNodes: data parent node" << tempNextParentNodePt->getService() <<  "at address" << tempParentAddress.toHex() << "has incorrect previous address:" << tempNextParentNodePt->getPreviousParentAddress().toHex() << "instead of" << MPNode::EmptyAddress.toHex();
                        if (repairAllowed)
                        {
                            tempNextParentNodePt->setPreviousParentAddress(MPNode::EmptyAddress);
                        }
                        return_bool = false;
                    }
                }
                else
                {
                    /* normal linked chain */
                    if ((!tempNextParentNodePt->getPreviousParentAddress().isNull() && tempNextParentNodePt->getPreviousParentAddress() != tempParentNodePt->getAddress()) || (tempNextParentNodePt->getPreviousParentAddress().isNull() && tempNextParentNodePt->getPreviousParentVirtualAddress() != tempParentNodePt->getVirtualAddress()))
                    {
                        qWarning() << "tagPointedNodes: data parent node" << tempNextParentNodePt->getService() <<  "at address" << tempParentAddress.toHex() << "has incorrect previous address:" << tempNextParentNodePt->getPreviousParentAddress().toHex() << "instead of" << tempParentNodePt->getAddress().toHex();
                        if (repairAllowed)
                        {
                            tempNextParentNodePt->setPreviousParentAddress(tempParentNodePt->getAddress(), tempParentNodePt->getVirtualAddress());
                        }
                        return_bool = false;
                    }
                }

                /* Set correct pointed node */
                tempParentNodePt = tempNextParentNodePt;

                /* tag parent */
                tempParentNodePt->setPointedToCheck();

                /* get first child */
                tempChildAddress = tempParentNodePt->getStartChildAddress();
                tempVirtualChildAddress = tempParentNodePt->getStartChildVirtualAddress();

                /* browse through all the children */
                while ((tempChildAddress != MPNode::EmptyAddress) || (tempChildAddress.isNull() && tempVirtualChildAddress != 0))
                {
                    /* Get pointer to the child node */
                    tempNextChildNodePt = findNodeWithAddressInList(dataChildNodes, tempChildAddress, tempVirtualChildAddress);

                    /* Check we could find child pointer */
                    if (!tempNextChildNodePt)
                    {
                        if ((!tempChildAddress.isNull() && tempChildAddress == tempParentNodePt->getStartChildAddress()) || (tempChildAddress.isNull() && tempVirtualChildAddress == tempParentNodePt->getStartChildVirtualAddress()))
                        {
                            qWarning() << "tagPointedNodes: couldn't find first data child node with address" << tempChildAddress.toHex() << " for parent " << tempParentNodePt->getService() << " in our list";
                        }
                        else
                        {
                            qWarning() << "tagPointedNodes: couldn't find data child node with address" << tempChildAddress.toHex() << " for parent " << tempParentNodePt->getService() << " in our list";
                        }
                        return_bool = false;

                        if (repairAllowed)
                        {
                            if ((!tempChildAddress.isNull() && tempChildAddress == tempParentNodePt->getStartChildAddress()) || (tempChildAddress.isNull() && tempVirtualChildAddress == tempParentNodePt->getStartChildVirtualAddress()))
                            {
                                // first child
                                tempParentNodePt->setStartChildAddress(MPNode::EmptyAddress);
                            }
                            else
                            {
                                // fix prev child
                                tempChildNodePt->setNextChildDataAddress(MPNode::EmptyAddress);
                            }
                        }

                        /* Loop to next parent */
                        tempChildAddress = MPNode::EmptyAddress;
                    }
                    else if (tempNextChildNodePt->getPointedToCheck())
                    {
                        /* Linked chain loop detected */
                        if ((!tempChildAddress.isNull() && tempChildAddress == tempParentNodePt->getStartChildAddress()) || (tempChildAddress.isNull() && tempVirtualChildAddress == tempParentNodePt->getStartChildVirtualAddress()))
                        {
                            qCritical() << "tagPointedNodes: data child node already pointed to: parent node with address" << tempParentAddress.toHex() << " points to child node with address" << tempChildAddress.toHex();
                            if (repairAllowed)
                            {
                                tempParentNodePt->setStartChildAddress(MPNode::EmptyAddress);
                            }
                        }
                        else
                        {
                            qCritical() << "tagPointedNodes: data child node loop has been detected: child node with address" << tempChildNodePt->getAddress().toHex() << " points to child node with address" << tempChildAddress.toHex();
                            if (repairAllowed)
                            {
                                tempChildNodePt->setNextChildDataAddress(MPNode::EmptyAddress);
                            }
                        }

                        /* Stop looping */
                        return_bool = false;
                        break;
                    }
                    else
                    {
                        /* Set correct pointed node */
                        tempChildNodePt = tempNextChildNodePt;

                        /* Tag child */
                        tempChildNodePt->setPointedToCheck();

                        /* Loop to next possible child */
                        tempChildAddress = tempChildNodePt->getNextChildDataAddress();
                        tempVirtualChildAddress = tempChildNodePt->getNextChildVirtualAddress();
                    }
                }

                /* get next parent address */
                tempParentAddress = tempParentNodePt->getNextParentAddress();
                tempVirtualParentAddress = tempParentNodePt->getNextParentVirtualAddress();
            }
        }
    }

    return return_bool;
}

bool MPDevice::deleteDataParentChilds(MPNode *parentNodePt)
{
    quint32 cur_child_addr_v = parentNodePt->getStartChildVirtualAddress();
    QByteArray cur_child_addr = parentNodePt->getStartChildAddress();

    /* Detag nodes */
    detagPointedNodes();

    /* Set empty child (required because of tests later */
    parentNodePt->setStartChildAddress(MPNode::EmptyAddress);

    /* Start following the chain */
    while ((cur_child_addr != MPNode::EmptyAddress) || (cur_child_addr.isNull() && cur_child_addr_v != 0))
    {
        MPNode* cur_child_pt;

        /* Find node in list */
        cur_child_pt = findNodeWithAddressInList(dataChildNodes, cur_child_addr, cur_child_addr_v);

        if (!cur_child_pt)
        {
            qDebug() << "Data child node can't be found in list!";
            return false;
        }
        else
        {
            /* Deleting child node at address */
            qDebug() << "Deleting child node at address" << cur_child_pt->getAddress().toHex();

            /* Follow the list */
            cur_child_addr = cur_child_pt->getNextChildDataAddress();
            cur_child_addr_v = cur_child_pt->getNextChildVirtualAddress();

            /* Delete node */
            parentNodePt->removeChild(cur_child_pt);
            dataChildNodes.removeOne(cur_child_pt);
        }
    }

    return true;
}


bool MPDevice::addOrphanParentChildsToDB(MPNode *parentNodePt, bool isDataParent, Common::AddressType addrType /*= Common::CRED_ADDR_IDX*/)
{
    quint32 cur_child_addr_v = parentNodePt->getStartChildVirtualAddress();
    QByteArray cur_child_addr = parentNodePt->getStartChildAddress();
    MPNode* prev_child_pt = nullptr;

    /* Tag nodes */
    tagPointedNodes(!isDataParent, isDataParent, false, addrType);

    /* Start following the chain */
    while ((cur_child_addr != MPNode::EmptyAddress) || (cur_child_addr.isNull() && cur_child_addr_v != 0))
    {
        MPNode* cur_child_pt;

        /* Find node in list */
        if (isDataParent)
        {
            cur_child_pt = findNodeWithAddressInList(dataChildNodes, cur_child_addr, cur_child_addr_v);
        }
        else
        {
            NodeList& childNodes = Common::CRED_ADDR_IDX == addrType ? loginChildNodes : webAuthnLoginChildNodes;
            cur_child_pt = findNodeWithAddressInList(childNodes, cur_child_addr, cur_child_addr_v);
        }

        if (!cur_child_pt)
        {
            qDebug() << "Child node can't be found in list!";

            if (!prev_child_pt)
            {
                /* first child */
                parentNodePt->setStartChildAddress(MPNode::EmptyAddress);
            }
            else
            {
                if (isDataParent)
                {
                    prev_child_pt->setNextChildDataAddress(MPNode::EmptyAddress);
                }
                else
                {
                    prev_child_pt->setNextChildAddress(MPNode::EmptyAddress);
                }
            }
            return true;
        }
        else if(cur_child_pt->getPointedToCheck())
        {
            qDebug() << "Arrived to a tagged children, we'll stop there";
            return true;
        }
        else
        {
            qCritical() << "Followed the linked list but arrived to a non tagged node... have no clue why";
            if (isDataParent)
            {
                prev_child_pt = cur_child_pt;
                cur_child_addr = cur_child_pt->getNextChildDataAddress();
                cur_child_addr_v = cur_child_pt->getNextChildVirtualAddress();
            }
            else
            {
                prev_child_pt = cur_child_pt;
                cur_child_addr = cur_child_pt->getNextChildAddress();
                cur_child_addr_v = cur_child_pt->getNextChildVirtualAddress();
            }
        }
    }

    return true;
}

/* Return success status */
bool MPDevice::addOrphanParentToDB(MPNode *parentNodePt, bool isDataParent, bool addPossibleChildren, Common::AddressType addrType /*= Common::CRED_ADDR_IDX*/)
{
    MPNode* prevNodePt = nullptr;
    MPNode* curNodePt = nullptr;
    quint32 curNodeAddrVirtual;
    QByteArray curNodeAddr;

    /* Tag nodes */
    if (!tagPointedNodes(!isDataParent, isDataParent, false, addrType))
    {
        qCritical() << "Can't add orphan parent to a corrupted DB, please run integrity check";
        return false;
    }

    /* Detag them */
    detagPointedNodes(addrType);

    /* Which list do we want to browse ? */
    if (isDataParent)
    {
        curNodeAddr = startDataNode;
        curNodeAddrVirtual = virtualDataStartNode;
    }
    else
    {
        curNodeAddr = startNode[addrType];
        curNodeAddrVirtual = virtualStartNode[addrType];
    }

    qInfo() << "Adding parent node" << parentNodePt->getService();

    if (parentNodePt->getPointedToCheck())
    {
        qCritical() << "addParentOrphan: parent node" << parentNodePt->getService() << "is already pointed to";
        tagPointedNodes(!isDataParent, isDataParent, false, addrType);
        if (addPossibleChildren)
        {
            addOrphanParentChildsToDB(parentNodePt, isDataParent, addrType);
        }
        return true;
    }
    else
    {
        /* Check for Empty DB */
        if ((curNodeAddr == MPNode::EmptyAddress) || (curNodeAddr.isNull() && curNodeAddrVirtual == 0))
        {
            /* Update starting parent */
            if (isDataParent)
            {
                startDataNode = parentNodePt->getAddress();
                virtualDataStartNode = parentNodePt->getVirtualAddress();
            }
            else
            {
                startNode[addrType] = parentNodePt->getAddress();
                virtualStartNode[addrType] = parentNodePt->getVirtualAddress();
            }

            /* Update prev/next fields */
            parentNodePt->setPreviousParentAddress(MPNode::EmptyAddress, 0);
            parentNodePt->setNextParentAddress(MPNode::EmptyAddress, 0);
            tagPointedNodes(!isDataParent, isDataParent, false, addrType);
            if (addPossibleChildren)
            {
                addOrphanParentChildsToDB(parentNodePt, isDataParent, addrType);
            }
            return true;
        }

        /* browse through all the children to find the right slot */
        while ((curNodeAddr != MPNode::EmptyAddress) || (curNodeAddr.isNull() && curNodeAddrVirtual != 0))
        {
            /* Look for node in list */
            if (isDataParent)
            {
                curNodePt = findNodeWithAddressInList(dataNodes, curNodeAddr, curNodeAddrVirtual);
            }
            else
            {
                NodeList& nodes = addrType == Common::CRED_ADDR_IDX ? loginNodes : webAuthnLoginNodes;
                curNodePt = findNodeWithAddressInList(nodes, curNodeAddr, curNodeAddrVirtual);
            }

            /* Check if we could find the parent */
            if (!curNodePt)
            {
                qCritical() << "Broken parent linked list, please run integrity check";
                tagPointedNodes(!isDataParent, isDataParent, false, addrType);
                return false;
            }
            else
            {
                /* Check for tagged to avoid loops */
                if (curNodePt->getPointedToCheck())
                {
                    qCritical() << "Linked list loop detected, please run integrity check";
                    tagPointedNodes(!isDataParent, isDataParent, false, addrType);
                    return false;
                }

                /* Tag node */
                curNodePt->setPointedToCheck();

                /* If the login name is supposed to be before the one we're trying to insert */
                if (curNodePt->getService().compare(parentNodePt->getService()) > 0)
                {
                    /* We went one slot too far, curNodePt is the next parent Node */
                    qDebug() << "Adding parent node before" << curNodePt->getService();

                    /* Check if it is the new first parent */
                    if (!prevNodePt)
                    {
                        /* We have a new start node! */
                        qDebug() << "Parent node is the new start node";
                        if (isDataParent)
                        {
                            startDataNode = parentNodePt->getAddress();
                            virtualDataStartNode = parentNodePt->getVirtualAddress();
                        }
                        else
                        {
                            startNode[addrType] = parentNodePt->getAddress();
                            virtualStartNode[addrType] = parentNodePt->getVirtualAddress();
                        }
                        parentNodePt->setPreviousParentAddress(MPNode::EmptyAddress);
                    }
                    else
                    {
                        qDebug() << "... and after" << prevNodePt->getService();
                        prevNodePt->setNextParentAddress(parentNodePt->getAddress(), parentNodePt->getVirtualAddress());
                        parentNodePt->setPreviousParentAddress(prevNodePt->getAddress(), prevNodePt->getVirtualAddress());
                    }

                    /* Update our next node pointers */
                    curNodePt->setPreviousParentAddress(parentNodePt->getAddress(), parentNodePt->getVirtualAddress());
                    parentNodePt->setNextParentAddress(curNodePt->getAddress(), curNodePt->getVirtualAddress());
                    tagPointedNodes(!isDataParent, isDataParent, false, addrType);
                    if (addPossibleChildren)
                    {
                        addOrphanParentChildsToDB(parentNodePt, isDataParent, addrType);
                    }
                    return true;
                }
            }

            /* Set correct pointed node */
            prevNodePt = curNodePt;

            /* Loop to next possible child */
            curNodeAddr = curNodePt->getNextParentAddress();
            curNodeAddrVirtual = curNodePt->getNextParentVirtualAddress();
        }

        if (!prevNodePt) {
            qCritical() << "There is no last node to use!";
            return false;
        }

        /* If we are here it means we need to take the last spot */
        qDebug() << "Adding parent node after" << prevNodePt->getService();
        prevNodePt->setNextParentAddress(parentNodePt->getAddress(), parentNodePt->getVirtualAddress());
        parentNodePt->setPreviousParentAddress(prevNodePt->getAddress(), prevNodePt->getVirtualAddress());
        parentNodePt->setNextParentAddress(MPNode::EmptyAddress);
        tagPointedNodes(!isDataParent, isDataParent, false, addrType);
        if (addPossibleChildren)
        {
            addOrphanParentChildsToDB(parentNodePt, isDataParent, addrType);
        }
        return true;
    }
}

MPNode* MPDevice::addNewServiceToDB(const QString &service, Common::AddressType addrType /*= Common::CRED_ADDR_IDX*/)
{
    MPNode* tempNodePt;
    MPNode* newNodePt;

    qDebug() << "Creating new service" << service << "in DB";

    /* Does the service actually exist? */
    tempNodePt = findNodeWithServiceInList(service, addrType);
    if (tempNodePt)
    {
        qCritical() << "Service already exists.... dumbass!";
        return nullptr;
    }

    /* Increment new addresses counter */
    incrementNeededAddresses(MPNode::NodeParent);

    /* Create new node with null address and virtual address set to our counter value */
    newNodePt = pMesProt->createMPNode(QByteArray(getParentNodeSize(), 0), this, QByteArray(), newAddressesNeededCounter);
    newNodePt->setType(MPNode::NodeParent);
    newNodePt->setService(service);

    NodeList& nodes = addrType == Common::CRED_ADDR_IDX ? loginNodes : webAuthnLoginNodes;
    /* Add node to list */
    nodes.append(newNodePt);
    addOrphanParentToDB(newNodePt, false, false, addrType);

    return newNodePt;
}

bool MPDevice::addChildToDB(MPNode* parentNodePt, MPNode* childNodePt, Common::AddressType addrType /*= Common::CRED_ADDR_IDX*/)
{
    qInfo() << "Adding child " << childNodePt->getLogin() << " with parent " << parentNodePt->getService() << " to DB";
    MPNode* tempChildNodePt = nullptr;
    MPNode* tempNextChildNodePt;

    /* Detag nodes */
    detagPointedNodes(addrType);

    /* Get first child */
    QByteArray tempChildAddress = parentNodePt->getStartChildAddress();
    quint32 tempVirtualChildAddress = parentNodePt->getStartChildVirtualAddress();

    /* Check for Empty address */
    if ((tempChildAddress == MPNode::EmptyAddress) || (tempChildAddress.isNull() && tempVirtualChildAddress == 0))
    {
        parentNodePt->setStartChildAddress(childNodePt->getAddress(), childNodePt->getVirtualAddress());
        childNodePt->setPreviousChildAddress(MPNode::EmptyAddress);
        childNodePt->setNextChildAddress(MPNode::EmptyAddress);
        parentNodePt->appendChild(childNodePt);
        tagPointedNodes(true, false, false, addrType);
        return true;
    }

    NodeList& childNodes = addrType == Common::CRED_ADDR_IDX ? loginChildNodes : webAuthnLoginChildNodes;
    /* browse through all the children to find the right slot */
    while ((tempChildAddress != MPNode::EmptyAddress) || (tempChildAddress.isNull() && tempVirtualChildAddress != 0))
    {
        /* Get pointer to the child node */
        tempNextChildNodePt = findNodeWithAddressInList(childNodes, tempChildAddress, tempVirtualChildAddress);

        /* Check we could find child pointer */
        if (!tempNextChildNodePt)
        {
            qCritical() << "Broken child linked list, please run integrity check";
            tagPointedNodes(true, false, false, addrType);
            return false;
        }
        else
        {
            /* Check for tagged to avoid loops */
            if (tempNextChildNodePt->getPointedToCheck())
            {
                qCritical() << "Linked list loop detected, please run integrity check";
                tagPointedNodes(true, false, false, addrType);
                return false;
            }

            /* Tag node */
            tempNextChildNodePt->setPointedToCheck();

            if (tempNextChildNodePt->getLogin().compare(childNodePt->getLogin()) == 0)
            {
                qCritical() << "Can't add child node that has the exact same name!";
                tagPointedNodes(true, false, false, addrType);
                return false;
            }
            else if (tempNextChildNodePt->getLogin().compare(childNodePt->getLogin()) > 0)
            {
                /* If the login name is supposed to be before the one we're trying to insert */

                /* Check if the current login is the first one */
                if ((tempChildAddress == parentNodePt->getStartChildAddress()) && (tempVirtualChildAddress == parentNodePt->getStartChildVirtualAddress()))
                {
                    qDebug() << "Added in first position";
                    parentNodePt->setStartChildAddress(childNodePt->getAddress(), childNodePt->getVirtualAddress());
                    childNodePt->setPreviousChildAddress(MPNode::EmptyAddress);
                    childNodePt->setNextChildAddress(tempNextChildNodePt->getAddress(), tempNextChildNodePt->getVirtualAddress());
                    tempNextChildNodePt->setPreviousChildAddress(childNodePt->getAddress(), childNodePt->getVirtualAddress());
                    parentNodePt->appendChild(childNodePt);
                    tagPointedNodes(true, false, false, addrType);
                    return true;
                }
                else
                {
                    qDebug() << "Added in middle of list";
                    tempChildNodePt->setNextChildAddress(childNodePt->getAddress(), childNodePt->getVirtualAddress());
                    childNodePt->setPreviousChildAddress(tempChildNodePt->getAddress(), tempChildNodePt->getVirtualAddress());
                    childNodePt->setNextChildAddress(tempNextChildNodePt->getAddress(), tempNextChildNodePt->getVirtualAddress());
                    tempNextChildNodePt->setPreviousChildAddress(childNodePt->getAddress(), childNodePt->getVirtualAddress());
                    parentNodePt->appendChild(childNodePt);
                    tagPointedNodes(true, false, false, addrType);
                    return true;
                }
            }

            /* Set correct pointed node */
            tempChildNodePt = tempNextChildNodePt;

            /* Loop to next possible child */
            tempChildAddress = tempChildNodePt->getNextChildAddress();
            tempVirtualChildAddress = tempChildNodePt->getNextChildVirtualAddress();
        }
    }

    /* If we arrived here, it means we are the last node */
    qDebug() << "Added in last position";
    tempChildNodePt->setNextChildAddress(childNodePt->getAddress(), childNodePt->getVirtualAddress());
    childNodePt->setPreviousChildAddress(tempChildNodePt->getAddress(), tempChildNodePt->getVirtualAddress());
    childNodePt->setNextChildAddress(MPNode::EmptyAddress);
    parentNodePt->appendChild(childNodePt);
    tagPointedNodes(true, false, false, addrType);
    return true;
}

bool MPDevice::removeEmptyParentFromDB(MPNode* parentNodePt, bool isDataParent, Common::AddressType addrType /*= Common::CRED_ADDR_IDX*/)
{
    qDebug() << "Removing parent " << parentNodePt->getService() << " from DB, addr: " << parentNodePt->getAddress().toHex();
    MPNode* nextNodePt = nullptr;
    MPNode* prevNodePt = nullptr;
    MPNode* curNodePt = nullptr;
    quint32 curNodeAddrVirtual;
    QByteArray curNodeAddr;
    const bool isCred = addrType == Common::CRED_ADDR_IDX;
    NodeList& nodes = isCred ? loginNodes : webAuthnLoginNodes;

    /* Tag nodes */
    if (!tagPointedNodes(!isDataParent, isDataParent, false, addrType))
    {
        qCritical() << "Can't remove parent to a corrupted DB, please run integrity check";
        return false;
    }

    /* Detag them */
    detagPointedNodes(addrType);

    /* Which list do we want to browse ? */
    if (isDataParent)
    {
        curNodeAddr = startDataNode;
        curNodeAddrVirtual = virtualDataStartNode;
    }
    else
    {
        curNodeAddr = startNode[addrType];
        curNodeAddrVirtual = virtualStartNode[addrType];
    }

    if (parentNodePt->getPointedToCheck())
    {
        qCritical() << "addParentOrphan: parent node" << parentNodePt->getService() << "is already pointed to";
        tagPointedNodes(!isDataParent, isDataParent, false, addrType);
        return true;
    }
    else
    {
        /* Check for Empty DB */
        if ((curNodeAddr == MPNode::EmptyAddress) || (curNodeAddr.isNull() && curNodeAddrVirtual == 0))
        {
            qCritical() << "Database is empty!";
            tagPointedNodes(!isDataParent, isDataParent, false, addrType);
            return false;
        }

        /* browse through all the parents to find the right slot */
        while ((curNodeAddr != MPNode::EmptyAddress) || (curNodeAddr.isNull() && curNodeAddrVirtual != 0))
        {
            /* Look for node in list */
            if (isDataParent)
            {
                curNodePt = findNodeWithAddressInList(dataNodes, curNodeAddr, curNodeAddrVirtual);
            }
            else
            {
                curNodePt = findNodeWithAddressInList(nodes, curNodeAddr, curNodeAddrVirtual);
            }

            /* Check if we could find the parent */
            if (!curNodePt)
            {
                qCritical() << "Broken parent linked list, please run integrity check";
                tagPointedNodes(!isDataParent, isDataParent, false, addrType);
                return false;
            }
            else
            {
                /* Check for tagged to avoid loops */
                if (curNodePt->getPointedToCheck())
                {
                    qCritical() << "Linked list loop detected, please run integrity check";
                    tagPointedNodes(!isDataParent, isDataParent, false, addrType);
                    return false;
                }

                /* Tag node */
                curNodePt->setPointedToCheck();

                /* Check for match */
                if (curNodePt == parentNodePt)
                {
                    /* Check if the parent actually doesn't have any child */
                    if ((curNodePt->getStartChildAddress() != MPNode::EmptyAddress) || (curNodePt->getStartChildAddress().isNull() && curNodePt->getStartChildVirtualAddress() != 0))
                    {
                        qCritical() << "Parent actually has a child!";
                        tagPointedNodes(!isDataParent, isDataParent, false, addrType);
                        return false;
                    }

                    /* Check if it is the last node */
                    bool isLastNode = ((parentNodePt->getNextParentAddress() == MPNode::EmptyAddress) || (parentNodePt->getNextParentAddress().isNull() && parentNodePt->getNextParentVirtualAddress() == 0));

                    /* If not the last node, get pointer to it */
                    if (!isLastNode)
                    {
                        /* Get next node pointer, if it exists */
                        if (isDataParent)
                        {
                            nextNodePt = findNodeWithAddressInList(dataNodes, parentNodePt->getNextParentAddress(), parentNodePt->getNextParentVirtualAddress());
                        }
                        else
                        {
                            nextNodePt = findNodeWithAddressInList(nodes, parentNodePt->getNextParentAddress(), parentNodePt->getNextParentVirtualAddress());
                        }

                        if (!nextNodePt)
                        {
                            qCritical() << "Broken parent linked list, please run integrity check";
                            tagPointedNodes(!isDataParent, isDataParent, false, addrType);
                            return false;
                        }
                    }

                    /* Check if it was first parent */
                    if (!prevNodePt)
                    {
                        qInfo() << "Parent node was start node";

                        if (isLastNode)
                        {
                            /* Linked list ends there, only credential */
                            if (isDataParent)
                            {
                                startDataNode = MPNode::EmptyAddress;
                                virtualDataStartNode = 0;
                            }
                            else
                            {
                                startNode[addrType] = MPNode::EmptyAddress;
                                virtualStartNode[addrType] = 0;
                            }
                        }
                        else
                        {
                            /* Link the chain together */
                            if (isDataParent)
                            {
                                startDataNode = nextNodePt->getAddress();
                                virtualDataStartNode = nextNodePt->getVirtualAddress();
                            }
                            else
                            {
                                startNode[addrType] = nextNodePt->getAddress();
                                virtualStartNode[addrType] = nextNodePt->getVirtualAddress();
                            }
                            nextNodePt->setPreviousParentAddress(MPNode::EmptyAddress, 0);
                        }
                        /* Delete object */
                        if (isDataParent)
                        {
                            dataNodes.removeOne(parentNodePt);
                        }
                        else
                        {
                            nodes.removeOne(parentNodePt);
                        }
                        delete parentNodePt;
                        tagPointedNodes(!isDataParent, isDataParent, false, addrType);
                        return true;
                    }
                    else
                    {
                        if (isLastNode)
                        {
                            /* Linked list ends there */
                            prevNodePt->setNextParentAddress(MPNode::EmptyAddress, 0);
                        }
                        else
                        {
                            /* Link the chain together */
                            prevNodePt->setNextParentAddress(nextNodePt->getAddress(), nextNodePt->getVirtualAddress());
                            nextNodePt->setPreviousParentAddress(prevNodePt->getAddress(), prevNodePt->getVirtualAddress());
                        }
                        /* Delete object */
                        if (isDataParent)
                        {
                            dataNodes.removeOne(parentNodePt);
                        }
                        else
                        {
                            nodes.removeOne(parentNodePt);
                        }
                        delete parentNodePt;
                        tagPointedNodes(!isDataParent, isDataParent, false, addrType);
                        return true;
                    }
                }
            }

            /* Set correct pointed node */
            prevNodePt = curNodePt;

            /* Loop to next possible child */
            curNodeAddr = curNodePt->getNextParentAddress();
            curNodeAddrVirtual = curNodePt->getNextParentVirtualAddress();
        }

        /* If we arrived here, it means we didn't find the parent */
        qCritical() << "Broken parent linked list, please run integrity check";
        tagPointedNodes(!isDataParent, isDataParent, false, addrType);
        return false;
    }
}

bool MPDevice::removeChildFromDB(MPNode* parentNodePt, MPNode* childNodePt, bool deleteEmptyParent, bool deleteFromList, Common::AddressType addrType /*= Common::CRED_ADDR_IDX*/)
{
    if (deleteFromList)
        qDebug() << "Removing child " << childNodePt->getLogin() << " with parent " << parentNodePt->getService() << " from DB";
    else
        qDebug() << "Removing child " << childNodePt->getLogin() << " from parent " << parentNodePt->getService();

    MPNode* tempChildNodePt = nullptr;
    MPNode* tempNextChildNodePt;

    /* Get first child */
    QByteArray tempChildAddress = parentNodePt->getStartChildAddress();
    quint32 tempVirtualChildAddress = parentNodePt->getStartChildVirtualAddress();

    /* Check for Empty address */
    if ((tempChildAddress == MPNode::EmptyAddress) || (tempChildAddress.isNull() && tempVirtualChildAddress == 0))
    {
        qCritical() << "The child doesn't exist in the parent listed childs (parent has no child)";
        return false;
    }

    const bool isCred = addrType == Common::CRED_ADDR_IDX;
    NodeList& childNodes = isCred? loginChildNodes : webAuthnLoginChildNodes;
    /* browse through all the children to find the right child */
    while ((tempChildAddress != MPNode::EmptyAddress) || (tempChildAddress.isNull() && tempVirtualChildAddress != 0))
    {
        /* Get pointer to the child node */
        tempNextChildNodePt = findNodeWithAddressInList(childNodes, tempChildAddress, tempVirtualChildAddress);

        /* Check we could find child pointer */
        if (!tempNextChildNodePt)
        {
            qCritical() << "Broken child linked list, please run integrity check";
            return false;
        }
        else
        {
            /* Same pointer? */
            if (tempNextChildNodePt == childNodePt)
            {
                /* Check if it is the last node */
                bool isLastNode = ((childNodePt->getNextChildAddress() == MPNode::EmptyAddress) || (childNodePt->getNextChildAddress().isNull() && childNodePt->getNextChildVirtualAddress() == 0));

                /* If not the last node, get pointer to it */
                if (!isLastNode)
                {
                    /* Get next node pointer, if it exists */
                    tempNextChildNodePt = findNodeWithAddressInList(childNodes, childNodePt->getNextChildAddress(), childNodePt->getNextChildVirtualAddress());

                    if (!tempNextChildNodePt)
                    {
                        qCritical() << "Broken child linked list, please run integrity check";
                        return false;
                    }
                }

                /* Check if the current login is the first one */
                if ((tempChildAddress == parentNodePt->getStartChildAddress()) && (tempVirtualChildAddress == parentNodePt->getStartChildVirtualAddress()))
                {
                    if (isLastNode)
                    {
                        /* Linked list ends there, only credential */
                        parentNodePt->setStartChildAddress(MPNode::EmptyAddress, 0);
                    }
                    else
                    {
                        /* Link the chain together */
                        parentNodePt->setStartChildAddress(tempNextChildNodePt->getAddress(), tempNextChildNodePt->getVirtualAddress());
                        tempNextChildNodePt->setPreviousChildAddress(MPNode::EmptyAddress, 0);
                    }

                    if (isCred)
                    {
                        /* Delete possible fav */
                        deletePossibleFavorite(parentNodePt->getAddress(), childNodePt->getAddress());
                    }

                    /* Delete object */
                    parentNodePt->removeChild(childNodePt);
                    if (deleteFromList)
                    {
                        childNodes.removeOne(childNodePt);
                        delete childNodePt;
                    }

                    /* Remove parent */
                    if (isLastNode && deleteEmptyParent)
                    {
                        removeEmptyParentFromDB(parentNodePt, false, addrType);
                    }
                    return true;
                }
                else
                {
                    if (isLastNode)
                    {
                        /* Linked list ends there */
                        tempChildNodePt->setNextChildAddress(MPNode::EmptyAddress, 0);
                    }
                    else
                    {
                        /* Link the chain together */
                        tempChildNodePt->setNextChildAddress(tempNextChildNodePt->getAddress(), tempNextChildNodePt->getVirtualAddress());
                        tempNextChildNodePt->setPreviousChildAddress(tempChildNodePt->getAddress(), tempChildNodePt->getVirtualAddress());
                    }

                    if (isCred)
                    {
                        /* Delete possible fav */
                        deletePossibleFavorite(parentNodePt->getAddress(), childNodePt->getAddress());
                    }

                    /* Delete object */
                    parentNodePt->removeChild(childNodePt);
                    if (deleteFromList)
                    {
                        loginChildNodes.removeOne(childNodePt);
                        delete childNodePt;
                    }
                    return true;
                }
            }

            /* Set correct pointed node */
            tempChildNodePt = tempNextChildNodePt;

            /* Loop to next possible child */
            tempChildAddress = tempChildNodePt->getNextChildAddress();
            tempVirtualChildAddress = tempChildNodePt->getNextChildVirtualAddress();
        }
    }

    /* If we arrived here, it means we didn't find the child */
    qCritical() << "Broken child linked list, please run integrity check";
    return false;
}

bool MPDevice::addOrphanChildToDB(MPNode* childNodePt, Common::AddressType addrType /*= Common::CRED_ADDR_IDX*/)
{
    QString recovered_service_name = "_recovered_";
    MPNode* tempNodePt;

    qInfo() << "Adding orphan child" << childNodePt->getLogin() << "to DB";

    /* Create a "_recovered_" service */
    tempNodePt = findNodeWithServiceInList(recovered_service_name, addrType);
    if (!tempNodePt)
    {
        qInfo() << "No" << recovered_service_name << "service in DB, adding it...";
        tempNodePt = addNewServiceToDB(recovered_service_name, addrType);
    }

    /* Add child to DB */
    return addChildToDB(tempNodePt, childNodePt, addrType);
}

bool MPDevice::checkLoadedNodes(bool checkCredentials, bool checkData, bool repairAllowed)
{
    QByteArray temp_pnode_address, temp_cnode_address;
    bool return_bool;

    qInfo() << "Checking database...";

    /* Tag pointed nodes, also detects DB errors */
    return_bool = tagPointedNodes(checkCredentials, checkData, repairAllowed);
    return_bool &= tagPointedNodes(checkCredentials, checkData, repairAllowed, Common::WEBAUTHN_ADDR_IDX);

    /* Scan for orphan nodes */
    quint32 nbOrphanParents = 0;
    quint32 nbOrphanChildren = 0;
    quint32 nbOrphanDataParents = 0;
    quint32 nbOrphanDataChildren = 0;
    quint32 nbWebauthnOrphanParents = 0;
    quint32 nbWebauthnOrphanChildren = 0;

    if (checkCredentials)
    {
        checkLoadedLoginNodes(nbOrphanParents, nbOrphanChildren, repairAllowed, Common::CRED_ADDR_IDX);
        if (isBLE())
        {
           checkLoadedLoginNodes(nbWebauthnOrphanParents, nbWebauthnOrphanChildren, repairAllowed, Common::WEBAUTHN_ADDR_IDX);
        }
    }

    if (checkData)
    {
        if (checkCredentials && !tagPointedNodes(checkCredentials, checkData, repairAllowed))
        {
            /* Functions inside the code above may tag again the nodes, so we need to tag again */
            return_bool = false;
        }

        for (auto &i: dataNodes)
        {
            if (!i->getPointedToCheck())
            {
                qWarning() << "Orphan data parent found:" << i->getService() << "at address:" << i->getAddress().toHex();
                if (repairAllowed)
                {
                    addOrphanParentToDB(i, true, repairAllowed);
                }
                nbOrphanDataParents++;
            }
        }
        QListIterator<MPNode*> i(dataChildNodes);
        while (i.hasNext())
        {
            MPNode* nodeItem = i.next();
            if (!nodeItem->getPointedToCheck())
            {
                qWarning() << "Orphan data child found at address:" << nodeItem->getAddress().toHex();
                if (repairAllowed)
                {
                    /* no other choice than to delete them (the ctr is in the parent node */
                    qDebug() << "Removing data child at address:" << nodeItem->getAddress().toHex();
                    dataChildNodes.removeOne(nodeItem);
                }
                nbOrphanDataChildren++;
            }
        }
    }

    qDebug() << "Number of parent orphans:" << nbOrphanParents;
    qDebug() << "Number of children orphans:" << nbOrphanChildren;
    qDebug() << "Number of data parent orphans:" << nbOrphanDataParents;
    qDebug() << "Number of data children orphans:" << nbOrphanDataChildren;
    if (isBLE())
    {
        qDebug() << "Number of webauthn parent orphans:" << nbWebauthnOrphanParents;
        qDebug() << "Number of webauthn children orphans:" << nbWebauthnOrphanChildren;
    }

    if (checkCredentials)
    {
        /* Last step : check favorites */
        for (auto &i: favoritesAddrs)
        {
            /* Extract parent & child addresses */
            temp_pnode_address = i.mid(0, 2);
            temp_cnode_address = i.mid(2, 2);

            /* Check if favorite is set */
            if (temp_pnode_address != MPNode::EmptyAddress)
            {
                /* Find the nodes in memory */
                MPNode *temp_cnode_pointer = nullptr;
                MPNode *temp_pnode_pointer = findNodeWithAddressInList(loginNodes, temp_pnode_address, 0);
                if (temp_pnode_pointer)
                {
                    temp_cnode_pointer = findNodeWithAddressWithGivenParentInList(loginChildNodes,  temp_pnode_pointer, temp_cnode_address, 0);
                }

                if ((!temp_cnode_pointer) || (!temp_pnode_pointer))
                {
                    qCritical() << "Favorite is pointing to incorrect node!";
                    for (qint32 j = 0; j < i.length(); j++) i[j] = 0;
                }
            }
        }
    }

    /* Set return bool */
    if (nbOrphanParents+nbOrphanChildren+nbOrphanDataParents+nbOrphanDataChildren)
    {
        return_bool = false;
    }

    if (return_bool)
    {
        qInfo() << "Database check OK";
    }
    else
    {
        if (!repairAllowed)
        {
            qInfo() << "Errors were found in the database";
        }
        else
        {
            qInfo() << "Modifications made to the db, double checking them...";

            if (!checkLoadedNodes(checkCredentials, checkData, false))
            {
                qCritical() << "Double checking repairs failed... Mathieu, you s*ck!";
            }
            else
            {
                qInfo() << "DB corrections were successfully checked";
            }

            qInfo() << "Errors were found and corrected in the database";
        }
    }

    return return_bool;
}

void MPDevice::checkLoadedLoginNodes(quint32 &parentNum, quint32 &childNum, bool repairAllowed, Common::AddressType addrType)
{
    const bool isCred = addrType == Common::CRED_ADDR_IDX;
    NodeList& nodes = isCred ? loginNodes : webAuthnLoginNodes;
    NodeList& childNodes = isCred? loginChildNodes : webAuthnLoginChildNodes;
    for (auto &i: nodes)
    {
        if (!i->getPointedToCheck())
        {
            qWarning() << "Orphan parent found:" << i->getService() << "at address:" << i->getAddress().toHex();
            if (repairAllowed)
            {
                addOrphanParentToDB(i, false, repairAllowed, addrType);
            }
            parentNum++;
        }
    }
    for (auto &i: childNodes)
    {
        if (!i->getPointedToCheck())
        {
            qWarning() << "Orphan child found:" << i->getLogin() << "at address:" << i->getAddress().toHex();
            if (repairAllowed)
            {
                quint32 append_number = 2;
                QString service_name = i->getLogin();

                /* If the "_recovered_" service already has the same login, append a number */
                while(!addOrphanChildToDB(i))
                {
                    i->setLogin(service_name + QString::number(append_number));
                    append_number++;
                }
            }
            childNum++;
        }
    }
}

void MPDevice::addWriteNodePacketToJob(AsyncJobs *jobs, const QByteArray& address, const QByteArray& data, std::function<void(void)> writeCallback)
{
    for (auto packet : pMesProt->createWriteNodePackets(data, address))
    {
        jobs->append(new MPCommandJob(this, MPCmd::WRITE_FLASH_NODE, packet,
            [this, writeCallback](const QByteArray &data, bool &) -> bool
        {
            if (pMesProt->getFirstPayloadByte(data) == 0)
            {
                qCritical() << "Couldn't Write In Flash";
                return false;
            }
            else
            {
                writeCallback();
                return true;
            }
        }));

        if (AppDaemon::isDebugDev())
            qDebug() << "Write node packet #" << static_cast<quint8>(packet[2]) << " : " << packet.toHex();
    }
}

/* Return true if packets need to be sent */
bool MPDevice::generateSavePackets(AsyncJobs *jobs, bool tackleCreds, bool tackleData, const MPDeviceProgressCb &cbProgress)
{
    qInfo() << "Generating Save Packets...";
    bool diagSavePacketsGenerated = false;
    MPNode* temp_node_pointer;
    progressCurrent = 0;
    progressTotal = 0;

    auto dataWriteProgressCb = [this, cbProgress](void)
    {
        QVariantMap data = {
            {"total", progressTotal},
            {"current", ++progressCurrent},
            {"msg", "Writing Data To Device: %1/%2 Packets Sent" },
            {"msg_args", QVariantList({progressCurrent, progressTotal})}
        };
        cbProgress(data);
    };

    /* Change numbers */
    if (isFw12() || isBLE())
    {
        if ((get_credentialsDbChangeNumber() != credentialsDbChangeNumberClone) || (get_dataDbChangeNumber() != dataDbChangeNumberClone))
        {
            diagSavePacketsGenerated = true;
            qDebug() << "Updating cred & data change numbers";
            qDebug() << "Cred DB: " << get_credentialsDbChangeNumber() << " clone: " << credentialsDbChangeNumberClone;
            qDebug() << "Data cred DB: " << get_dataDbChangeNumber() << " clone: " << dataDbChangeNumberClone;
            updateChangeNumbers(jobs, Common::CredentialNumberChanged|Common::DataNumberChanged);
        }
    }

    /* First pass: check the nodes that changed or were added */
    if (tackleCreds)
    {
        diagSavePacketsGenerated |= checkModifiedSavePacketNodes(jobs, dataWriteProgressCb, Common::CRED_ADDR_IDX);
        if (isBLE())
        {
            diagSavePacketsGenerated |= checkModifiedSavePacketNodes(jobs, dataWriteProgressCb, Common::WEBAUTHN_ADDR_IDX);
        }
    }

    if (tackleData)
    {
        for (auto &nodelist_iterator: dataNodes)
        {
            /* See if we can find the same node in the clone list */
            temp_node_pointer = findNodeWithAddressInList(dataNodesClone, nodelist_iterator->getAddress(), 0);

            if (!temp_node_pointer)
            {
                qDebug() << "Generating save packet for new data service" << nodelist_iterator->getService();
                addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), nodelist_iterator->getNodeData(), dataWriteProgressCb);
                diagSavePacketsGenerated = true;
                progressTotal += 3;
            }
            else if (nodelist_iterator->getNodeData() != temp_node_pointer->getNodeData())
            {
                qDebug() << "Generating save packet for updated data service" << nodelist_iterator->getService();
                addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), nodelist_iterator->getNodeData(), dataWriteProgressCb);
                diagSavePacketsGenerated = true;
                progressTotal += 3;
            }
        }
        for (auto &nodelist_iterator: dataChildNodes)
        {
            /* See if we can find the same node in the clone list */
            temp_node_pointer = findNodeWithAddressInList(dataChildNodesClone, nodelist_iterator->getAddress(), 0);

            if (!temp_node_pointer)
            {
                qDebug() << "Generating save packet for new data child node";
                addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), nodelist_iterator->getNodeData(), dataWriteProgressCb);
                diagSavePacketsGenerated = true;
                progressTotal += 3;
            }
            else if (nodelist_iterator->getNodeData() != temp_node_pointer->getNodeData())
            {
                qDebug() << "Generating save packet for updated data child node";
                qDebug() << "Prev contents: " << temp_node_pointer->getNodeData().toHex();
                qDebug() << "New  contents: " << nodelist_iterator->getNodeData().toHex();
                addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), nodelist_iterator->getNodeData(), dataWriteProgressCb);
                diagSavePacketsGenerated = true;
                progressTotal += 3;
            }
        }
    }

    /* Second pass: check the nodes that were removed */
    if (tackleCreds)
    {
        diagSavePacketsGenerated |= checkRemovedSavePacketNodes(jobs, dataWriteProgressCb, Common::CRED_ADDR_IDX);
        if (isBLE())
        {
            diagSavePacketsGenerated |= checkRemovedSavePacketNodes(jobs, dataWriteProgressCb, Common::WEBAUTHN_ADDR_IDX);
        }

        /* Diff favorites */
        for (qint32 i = 0; i < favoritesAddrs.length(); i++)
        {
            if (favoritesAddrs[i] != favoritesAddrsClone[i])
            {
                qDebug() << "Generating favorite" << i << "update packet";
                diagSavePacketsGenerated = true;
                QByteArray updateFavPacket;
                if (isBLE())
                {
                    updateFavPacket.append(pMesProt->toLittleEndianFromInt(i/MAX_BLE_CAT_NUM));
                    updateFavPacket.append(pMesProt->toLittleEndianFromInt(i%MAX_BLE_CAT_NUM));
                }
                else
                {
                    updateFavPacket.append(i);
                }
                updateFavPacket.append(favoritesAddrs[i]);
                jobs->append(new MPCommandJob(this, MPCmd::SET_FAVORITE, updateFavPacket, pMesProt->getDefaultFuncDone()));
            }
        }

        /* Diff start node */
        if (startNode[Common::CRED_ADDR_IDX] != startNodeClone[Common::CRED_ADDR_IDX])
        {
            qDebug() << "Updating start node";
            diagSavePacketsGenerated = true;
            QByteArray setAddress;
            if (isBLE())
            {
                setAddress = bleImpl->getStartAddressToSet(startNode, Common::CRED_ADDR_IDX);
            }
            else
            {
                setAddress = startNode[Common::CRED_ADDR_IDX];
            }
            jobs->append(new MPCommandJob(this, MPCmd::SET_STARTING_PARENT, setAddress, pMesProt->getDefaultFuncDone()));
        }

        if (isBLE() && startNode[Common::WEBAUTHN_ADDR_IDX] != startNodeClone[Common::WEBAUTHN_ADDR_IDX])
        {
            qDebug() << "Updating start node";
            diagSavePacketsGenerated = true;
            QByteArray setAddress = bleImpl->getStartAddressToSet(startNode, Common::WEBAUTHN_ADDR_IDX);
            jobs->append(new MPCommandJob(this, MPCmd::SET_STARTING_PARENT, setAddress, pMesProt->getDefaultFuncDone()));
        }
    }
    if (tackleData)
    {
        for (auto &nodelist_iterator: dataNodesClone)
        {
            /* See if we can find the same node in the clone list */
            temp_node_pointer = findNodeWithAddressInList(dataNodes, nodelist_iterator->getAddress(), 0);

            if (!temp_node_pointer)
            {
                qDebug() << "Generating delete packet for deleted data service" << nodelist_iterator->getService();
                addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), QByteArray(pMesProt->getParentNodeSize(), 0xFF), dataWriteProgressCb);
                diagSavePacketsGenerated = true;
                progressTotal += 3;
            }
        }
        for (auto &nodelist_iterator: dataChildNodesClone)
        {
            /* See if we can find the same node in the clone list */
            temp_node_pointer = findNodeWithAddressInList(dataChildNodes, nodelist_iterator->getAddress(), 0);

            if (!temp_node_pointer)
            {
                qDebug() << "Generating delete packet for deleted data child node";
                addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), QByteArray(pMesProt->getChildNodeSize(), 0xFF), dataWriteProgressCb);
                diagSavePacketsGenerated = true;
                progressTotal += 3;
            }
        }

        /* Diff start data node */
        if (startDataNode != startDataNodeClone)
        {
            qDebug() << "Updating start data node";
            diagSavePacketsGenerated = true;
            QByteArray startData;
            if (isBLE())
            {
                const auto ZERO_BYTE = static_cast<char>(0);
                Common::fill(startData, 2, ZERO_BYTE);
            }
            startData.append(startDataNode);
            jobs->append(new MPCommandJob(this, MPCmd::SET_DN_START_PARENT, startData, pMesProt->getDefaultFuncDone()));
        }
    }

    /* Diff ctr */
    if (ctrValue != ctrValueClone)
    {
        qDebug() << "Updating CTR value";
        diagSavePacketsGenerated = true;
        jobs->append(new MPCommandJob(this, MPCmd::SET_CTRVALUE, ctrValue, pMesProt->getDefaultFuncDone()));
    }

    /* We need to diff cpz ctr values for firmwares running < v1.2 */
    /* Diff: cpzctr values can only be added by design */
    if (!isBLE())
    {
        for (qint32 i = 0; i < cpzCtrValue.length(); i++)
        {
            bool cpzCtrFound = false;
            for (qint32 j = 0; j < cpzCtrValueClone.length(); j++)
            {
                if (cpzCtrValue[i] == cpzCtrValueClone[j])
                {
                    cpzCtrFound = true;
                    break;
                }
            }
            if (!cpzCtrFound)
            {
                qDebug() << "Adding missing cpzctr";
                diagSavePacketsGenerated = true;
                jobs->append(new MPCommandJob(this, MPCmd::ADD_CARD_CPZ_CTR, cpzCtrValue[i], pMesProt->getDefaultFuncDone()));
            }
        }
    }

    if (diagSavePacketsGenerated)
    {
        qInfo() << "Update packets were generated";
    }
    else
    {
        qInfo() << "No need to generate update packets";
    }

    return diagSavePacketsGenerated;
}

bool MPDevice::checkModifiedSavePacketNodes(AsyncJobs *jobs, std::function<void()> writeCb, Common::AddressType addrType)
{
    const bool isCred = addrType == Common::CRED_ADDR_IDX;
    NodeList& nodes = isCred ? loginNodes : webAuthnLoginNodes;
    NodeList& nodesClone = isCred ? loginNodesClone : webAuthnLoginNodesClone;
    NodeList& childNodes = isCred? loginChildNodes : webAuthnLoginChildNodes;
    NodeList& childNodesClone = isCred? loginChildNodesClone : webAuthnLoginChildNodesClone;
    MPNode* tmpNodePtr;
    bool savePacketGenerated = false;
    for (auto &nodelist_iterator: nodes)
    {
        /* See if we can find the same node in the clone list */
        tmpNodePtr = findNodeWithAddressInList(nodesClone, nodelist_iterator->getAddress(), 0);

        if (!tmpNodePtr)
        {
            qDebug() << "Generating save packet for new service" << nodelist_iterator->getService();
            //qDebug() << "New  contents: " << nodelist_iterator->getNodeData().toHex();
            addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), nodelist_iterator->getNodeData(), writeCb);
            savePacketGenerated = true;
            progressTotal += 3;
        }
        else if (nodelist_iterator->getNodeData() != tmpNodePtr->getNodeData())
        {
            qDebug() << "Generating save packet for updated service" << nodelist_iterator->getService();
            //qDebug() << "Prev contents: " << temp_node_pointer->getNodeData().toHex();
            //qDebug() << "New  contents: " << nodelist_iterator->getNodeData().toHex();
            addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), nodelist_iterator->getNodeData(), writeCb);
            savePacketGenerated = true;
            progressTotal += 3;
        }
    }
    for (auto &nodelist_iterator: childNodes)
    {
        /* See if we can find the same node in the clone list */
        tmpNodePtr = findNodeWithAddressInList(childNodesClone, nodelist_iterator->getAddress(), 0);

        if (!tmpNodePtr)
        {
            qDebug() << "Generating save packet for new login" << nodelist_iterator->getLogin();
            //qDebug() << "New  contents: " << nodelist_iterator->getNodeData().toHex();
            addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), nodelist_iterator->getNodeData(), writeCb);
            savePacketGenerated = true;
            progressTotal += 3;
        }
        else if (nodelist_iterator->getNodeData() != tmpNodePtr->getNodeData())
        {
            qDebug() << "Generating save packet for updated login" << nodelist_iterator->getLogin();
            addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), nodelist_iterator->getNodeData(), writeCb);
            savePacketGenerated = true;
            progressTotal += 3;
        }
    }
    return savePacketGenerated;
}

bool MPDevice::checkRemovedSavePacketNodes(AsyncJobs *jobs, std::function<void ()> writeCb, Common::AddressType addrType)
{
    const bool isCred = addrType == Common::CRED_ADDR_IDX;
    NodeList& nodes = isCred ? loginNodes : webAuthnLoginNodes;
    NodeList& nodesClone = isCred ? loginNodesClone : webAuthnLoginNodesClone;
    NodeList& childNodes = isCred? loginChildNodes : webAuthnLoginChildNodes;
    NodeList& childNodesClone = isCred? loginChildNodesClone : webAuthnLoginChildNodesClone;
    MPNode* tmpNodePtr;
    bool savePacketGenerated = false;
    for (auto &nodelist_iterator : nodesClone)
    {
        /* See if we can find the same node in the clone list */
        tmpNodePtr = findNodeWithAddressInList(nodes, nodelist_iterator->getAddress(), 0);

        if (!tmpNodePtr)
        {
            qDebug() << "Generating delete packet for deleted service" << nodelist_iterator->getService();
            addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), QByteArray(getParentNodeSize(), 0xFF), writeCb);
            savePacketGenerated = true;
            progressTotal += 3;
        }
    }
    for (auto &nodelist_iterator : childNodesClone)
    {
        /* See if we can find the same node in the clone list */
        tmpNodePtr = findNodeWithAddressInList(childNodes, nodelist_iterator->getAddress(), 0);

        if (!tmpNodePtr)
        {
            qDebug() << "Generating delete packet for deleted login" << nodelist_iterator->getLogin();
            addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), QByteArray(getChildNodeSize(), 0xFF), writeCb);
            savePacketGenerated = true;
            progressTotal += 3;
        }
    }
    return savePacketGenerated;
}

QByteArray MPDevice::getFreeAddress(quint32 virtualAddr)
{
    const int virtAddr = static_cast<int>(virtualAddr);
    if (isBLE())
    {
        return bleImpl->getFreeAddressProvider().getFreeAddress(virtAddr);
    }
    return freeAddresses[virtAddr];
}

void MPDevice::exitMemMgmtMode(bool setMMMBool)
{
    AsyncJobs *jobs = new AsyncJobs("Exiting MMM", this);

    jobs->append(new MPCommandJob(this, MPCmd::END_MEMORYMGMT, pMesProt->getDefaultFuncDone()));

    connect(jobs, &AsyncJobs::finished, [this, setMMMBool](const QByteArray &)
    {
        qInfo() << "MMM exit ok";
        cleanMMMVars();
        if (setMMMBool)
        {
            force_memMgmtMode(false);
        }
    });

    connect(jobs, &AsyncJobs::failed, [this, setMMMBool](AsyncJob *)
    {
        qCritical() << "Failed to exit MMM";
        cleanMMMVars();
        if (setMMMBool)
        {
            force_memMgmtMode(false);
        }
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::setCurrentDate()
{
    AsyncJobs *jobs = new AsyncJobs("Send date to device", this);

    jobs->append(new MPCommandJob(this, MPCmd::SET_DATE,
                                  [this](const QByteArray &, QByteArray &data_to_send) -> bool
    {
        data_to_send.clear();
        data_to_send.append(pMesProt->convertDate(QDateTime::currentDateTimeUtc()));

        qDebug() << "Sending current date: " << data_to_send.toHex();

        return true;
    },
                                [](const QByteArray &, bool &) -> bool
    {
        return true;
    }));

    connect(jobs, &AsyncJobs::finished, [](const QByteArray &)
    {
        qInfo() << "Date set success";
    });
    connect(jobs, &AsyncJobs::failed, [this](AsyncJob *)
    {
        qWarning() << "Failed to set date on device";
        if (!isBLE())
        {
            setCurrentDate(); // memory: does it get piled on?
        }
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}


void MPDevice::getUID(const QByteArray & key)
{
    AsyncJobs *jobs = new AsyncJobs("Send uid request to device", this);
    m_uid = -1;

    jobs->append(new MPCommandJob(this, MPCmd::GET_UID,
                                  [key](const QByteArray &, QByteArray &data_to_send) -> bool
    {
        bool ok;
        data_to_send.clear();
        data_to_send.resize(16);
        for(int i = 0; i < 16; i++) {
            data_to_send[i] = key.mid(2*i, 2).toUInt(&ok, 16);
            if(!ok)
                return false;
        }

        return true;
    },
                                [this](const QByteArray &data, bool &) -> bool
    {
        if (pMesProt->getMessageSize(data) == 1 )
        {
            qWarning() << "Couldn't request uid" << pMesProt->getFirstPayloadByte(data) << pMesProt->getMessageSize(data) << pMesProt->printCmd(data) << data.toHex();
            set_uid(-1);
            return false;
        }

        bool ok;
        quint64 uid = pMesProt->getFullPayload(data).toHex().toULongLong(&ok, 16);
        set_uid(ok ? uid : - 1);
        return ok;
    }));

    connect(jobs, &AsyncJobs::failed, [](AsyncJob *)
    {
        qWarning() << "Failed get uid from device";
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::processStatusChange(const QByteArray &data)
{
    /* Map status from received val */
    Common::MPStatus s = pMesProt->getStatus(data);
    Common::MPStatus prevStatus = get_status();

    if (isBLE() && Common::Unlocked == s)
    {
        bleImpl->readUserSettings(pMesProt->getPayloadBytes(data, 2, 4));
    }

    if (isBLE())
    {
        bleImpl->readBatteryPercent(data);
    }

    /* Trigger on status change */
    if (s != prevStatus)
    {
        qDebug() << "received MPCmd::MOOLTIPASS_STATUS: " << static_cast<int>(s);

        if (bleImpl)
        {
            bleImpl->checkNoBundle(s, prevStatus);
        }

        /* Update status */
        set_status(s);

        if (prevStatus == Common::UnknownStatus)
        {
            QTimer::singleShot(10, this, &MPDevice::sendSetupDeviceMessages);
        }

        if (s == Common::Unlocked || s == Common::UnknownSmartcard)
        {
            QTimer::singleShot(20, this, &MPDevice::getCurrentCardCPZ);
        }
        else
        {
            if (s != Common::MMMMode)
            {
                filesCache.resetState();
            }
        }

        if (s == Common::Unlocked)
        {
            /* If v1.2 firmware, query user change number */
            QTimer::singleShot(50, this, &MPDevice::handleDeviceUnlocked);
        }
    }
}

void MPDevice::getCurrentCardCPZ()
{
    AsyncJobs* cpzjobs = new AsyncJobs("Loading device card CPZ", this);

    /* Query change number */
    cpzjobs->append(new MPCommandJob(this,
                                  MPCmd::GET_CUR_CARD_CPZ,
                                  [this](const QByteArray &data, bool &) -> bool
    {
        if (pMesProt->isCPZInvalid(data))
        {
            qWarning() << "Couldn't request card CPZ";
            return false;
        }
        else
        {
            set_cardCPZ(pMesProt->getFullPayload(data));
            qDebug() << "Card CPZ: " << get_cardCPZ().toHex();
            if (filesCache.setCardCPZ(get_cardCPZ()))
            {
                qDebug() << "CPZ set to file cache, emitting file cache changed";
                emit filesCacheChanged();
            }
            return true;
        }
    }));

    connect(cpzjobs, &AsyncJobs::finished, [](const QByteArray &)
    {
        //data is last result
        //all jobs finished success
        qInfo() << "Finished loading card CPZ";
    });

    connect(cpzjobs, &AsyncJobs::failed, [this](AsyncJob *)
    {
        qCritical() << "Loading card CPZ failed";
        getCurrentCardCPZ();
    });

    jobsQueue.enqueue(cpzjobs);
    runAndDequeueJobs();
}

void MPDevice::sendSetupDeviceMessages()
{
    /* First start: load parameters */
    pSettings->loadParameters();
    setCurrentDate();
    if (isBLE())
    {
        setDateTimer = new QTimer{this};
        connect(setDateTimer, &QTimer::timeout, this, &MPDevice::setCurrentDate);
        setDateTimer->start(SET_DATE_INTERVAL);
    }
}

void MPDevice::handleDeviceUnlocked()
{
    if (isFw12() || isBLE())
    {
        qInfo() << "Requesting change numbers";
        getChangeNumbers();
        if (isBLE() && !bleImpl->areCategoriesFetched())
        {
            //Fetch category if it was not requested from Gui
            QTimer::singleShot(CATEGORY_FETCH_DELAY, bleImpl, &MPDeviceBleImpl::fetchCategories);
        }
    }
    else
    {
        qInfo() << "Firmware below v1.2, do not request change numbers";
    }
}

void MPDevice::getChangeNumbers()
{
    AsyncJobs* v12jobs = new AsyncJobs("Loading device db change numbers", this);

    /* Query change number */
    v12jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_USER_CHANGE_NB,
                                  [this](const QByteArray &data, bool &) -> bool
    {
        quint32 credDbChangeNum = 0;
        quint32 dataDbChangeNum = 0;
        if (!pMesProt->getChangeNumber(data, credDbChangeNum, dataDbChangeNum))
        {
            qWarning() << "Couldn't request change numbers";
            return false;
        }

        set_credentialsDbChangeNumber(credDbChangeNum);
        credentialsDbChangeNumberClone = credDbChangeNum;
        set_dataDbChangeNumber(dataDbChangeNum);
        dataDbChangeNumberClone = dataDbChangeNum;
        if (filesCache.setDbChangeNumber(dataDbChangeNum))
        {
            qDebug() << "dbChangeNumber set to file cache, emitting file cache changed";
            emit filesCacheChanged();
        }
        emit dbChangeNumbersChanged(credentialsDbChangeNumberClone, dataDbChangeNumberClone);
        qDebug() << "Credentials change number:" << get_credentialsDbChangeNumber();
        qDebug() << "Data change number:" << get_dataDbChangeNumber();
        return true;
    }));

    connect(v12jobs, &AsyncJobs::finished, [](const QByteArray &)
    {
        //data is last result
        //all jobs finished success
        qInfo() << "Finished loading change numbers";
    });

    connect(v12jobs, &AsyncJobs::failed, [this](AsyncJob *)
    {
        qCritical() << "Loading change numbers failed";
        getChangeNumbers(); // memory: does it get piled on?
    });

    jobsQueue.enqueue(v12jobs);
    runAndDequeueJobs();
}

void MPDevice::cancelUserRequest(const QString &reqid)
{
    // send data with platform code
    // This command is used to cancel a request.
    // As the request may block the sending queue, we directly send the command
    // and bypass the queue.

    if (!isFw12() && !isBLE())
    {
        qDebug() << "cancelUserRequest not supported for fw < 1.2";
        return;
    }

    qInfo() << "cancel user request (reqid: " << reqid << ")";

    if (currentJobs && currentJobs->getJobsId() == reqid)
    {
        qInfo() << "request_id match current one. Cancel current request";

        writeCancelRequest();
        return;
    }

    //search for an existing jobid in the queue.
    for (AsyncJobs *j: qAsConst(jobsQueue))
    {
        if (j->getJobsId() == reqid)
        {
            qInfo() << "Removing request from queue";
            jobsQueue.removeAll(j);
            return;
        }
    }

    qWarning() << "No request found for reqid: " << reqid;
}

void MPDevice::writeCancelRequest()
{
    const auto cancelRequestCmd = MPCmd::CANCEL_USER_REQUEST;
    QByteArray ba;
    ba.append(pMesProt->createPackets(QByteArray{}, cancelRequestCmd)[0]);

    qDebug() << "Platform send command: " << pMesProt->printCmd(cancelRequestCmd);

    if (AppDaemon::isDebugDev())
        qDebug() << "Message:" << ba.toHex();

    qDebug() << "Platform send command: " << QString("0x%1").arg(static_cast<quint8>(ba[1]), 2, 16, QChar('0'));
    if (isBLE())
    {
        bleImpl->flipMessageBit(ba);
    }
    platformWrite(ba);
}

void MPDevice::getCredential(QString service, const QString &login, const QString &fallback_service, const QString &reqid,
                             std::function<void(bool success, QString errstr, const QString &_service, const QString &login, const QString &pass, const QString &desc)> cb)
{
    QString logInf = QStringLiteral("Ask for password for service: %1 login: %2 fallback_service: %3 reqid: %4")
                     .arg(service)
                     .arg(login)
                     .arg(fallback_service)
                     .arg(reqid);

    AsyncJobs *jobs;
    if (reqid.isEmpty())
        jobs = new AsyncJobs(logInf, this);
    else
        jobs = new AsyncJobs(logInf, reqid, this);

    QByteArray sdata = pMesProt->toByteArray(service);
    sdata.append((char)0);

    jobs->append(new MPCommandJob(this, MPCmd::CONTEXT,
                                  sdata,
                                  [this, jobs, fallback_service, service](const QByteArray &data, bool &) -> bool
    {
        if (pMesProt->getFirstPayloadByte(data) != 1)
        {
            if (!fallback_service.isEmpty())
            {
                QByteArray fsdata = pMesProt->toByteArray(fallback_service);
                fsdata.append((char)0);
                jobs->prepend(new MPCommandJob(this, MPCmd::CONTEXT,
                                              fsdata,
                                              [this, jobs, fallback_service](const QByteArray &data, bool &) -> bool
                {
                    if (pMesProt->getFirstPayloadByte(data) != 1)
                    {
                        qWarning() << "Error setting context: " << pMesProt->getFirstPayloadByte(data);
                        jobs->setCurrentJobError("failed to select context and fallback_context on device");
                        return false;
                    }

                    QVariantMap m = {{ "service", fallback_service }};
                    jobs->user_data = m;

                    return true;
                }));
                return true;
            }

            qWarning() << "Error setting context: " << pMesProt->getFirstPayloadByte(data);
            jobs->setCurrentJobError("failed to select context on device");
            return false;
        }

        QVariantMap m = {{ "service", service }};
        jobs->user_data = m;

        return true;
    }));

    QByteArray ldata = pMesProt->toByteArray(login);
    if (!ldata.isEmpty())
        ldata.append((char)0);

    jobs->append(new MPCommandJob(this, MPCmd::GET_LOGIN,
                                  ldata,
                                  [this, jobs, login](const QByteArray &data, bool &) -> bool
    {
        if (pMesProt->getFirstPayloadByte(data) == 0 && !login.isEmpty())
        {
            jobs->setCurrentJobError("credential access refused by user");
            return false;
        }

        QString l = pMesProt->getFullPayload(data);
        if (!login.isEmpty() && l != login)
        {
            jobs->setCurrentJobError("login mismatch");
            return false;
        }

        QVariantMap m = jobs->user_data.toMap();
        m["login"] = l;
        jobs->user_data = m;

        return true;
    }));

    if (isFw12())
    {
        jobs->append(new MPCommandJob(this, MPCmd::GET_DESCRIPTION,
                                      [this, jobs](const QByteArray &data, bool &) -> bool
        {
            /// Commented below: in case there's an empty description it is impossible to distinguish between device refusal and empty description.
            /*if (data[MP_PAYLOAD_FIELD_INDEX] == 0)
            {
                jobs->setCurrentJobError("failed to query description on device");
                qWarning() << "failed to query description on device";
                return true; //Do not fail if description is not available for this node
            }*/
            QVariantMap m = jobs->user_data.toMap();
            m["description"] = pMesProt->getFullPayload(data);
            jobs->user_data = m;
            return true;
        }));
    }

    jobs->append(new MPCommandJob(this, MPCmd::GET_PASSWORD,
                                  [this, jobs](const QByteArray &data, bool &) -> bool
    {
        if (pMesProt->getFirstPayloadByte(data) == 0)
        {
            jobs->setCurrentJobError("failed to query password on device");
            return false;
        }
        return true;
    }));

    connect(jobs, &AsyncJobs::finished, [this, jobs, cb](const QByteArray &data)
    {
        //data is last result
        //all jobs finished success

        qInfo() << "Password retreived ok";
        QString pass = pMesProt->getFullPayload(data);

        QVariantMap m = jobs->user_data.toMap();
        cb(true, QString(), m["service"].toString(), m["login"].toString(), pass, m["description"].toString());
    });

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        qCritical() << "Failed getting password: " << failedJob->getErrorStr();
        cb(false, failedJob->getErrorStr(), QString(), QString(), QString(), QString());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::delCredentialAndLeave(QString service, const QString &login,
                                     const MPDeviceProgressCb &cbProgress,
                                     MessageHandlerCb cb)
{
    auto deleteCred = [this, service, login, cbProgress, cb]()
    {
        QJsonArray allCreds;
        bool found = false;
        foreach (MPNode *n, getLoginNodes())
        {
            if (n->getType() == MPNode::NodeParent)
            {
                foreach (MPNode *child, n->getChildNodes())
                {
                    if (n->getService() == service &&
                        child->getLogin() == login)
                    {
                        found = true;
                        continue;
                    }

                    QJsonObject o = {{ "address", QJsonArray({{ child->getAddress().at(0) },
                                                              { child->getAddress().at(1) }}) },
                                     { "description", child->getDescription() },
                                     { "favorite", child->getFavoriteProperty() },
                                     { "login", child->getLogin() },
                                     { "password", "" },
                                     { "service", n->getService() }};

                    allCreds.append(o);
                }
            }
        }

        if (!found)
        {
            exitMemMgmtMode();
            cb(false, "Credential was not found in database");
        }
        else
            setMMCredentials(allCreds, false, cbProgress, [this, cb](bool success, QString errstr)
            {
                exitMemMgmtMode(); //just in case
                cb(success, errstr);
            });
    };

    if (!get_memMgmtMode())
    {
        startMemMgmtMode(false,
                         cbProgress,
                         [cb, deleteCred](bool success, int, QString errMsg)
        {
            if (!success)
                cb(success, errMsg);
            else
                deleteCred();
        });
    }
    else
        deleteCred();
}

void MPDevice::getRandomNumber(std::function<void(bool success, QString errstr, const QByteArray &nums)> cb)
{
    AsyncJobs *jobs = new AsyncJobs("Get random numbers from device", this);

    auto cmd = new MPCommandJob(this, MPCmd::GET_RANDOM_NUMBER, QByteArray());
    cmd->setTimeout(5000);
    jobs->append(cmd);

    connect(jobs, &AsyncJobs::finished, [cb, this](const QByteArray &data)
    {
        //data is last result
        //all jobs finished success

        qInfo() << "Random numbers generated ok";
        cb(true, QString(), pMesProt->getPayloadBytes(data, 0, 32));
    });

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Failed generating rng";
        cb(false, "failed to generate random numbers", QByteArray());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::createJobAddContext(const QString &service, AsyncJobs *jobs, bool isDataNode)
{
    QByteArray sdata = pMesProt->toByteArray(service);
    sdata.append((char)0);

    quint8 cmdAddCtx = isDataNode?MPCmd::ADD_DATA_SERVICE:MPCmd::ADD_CONTEXT;
    quint8 cmdSelectCtx = isDataNode?MPCmd::SET_DATA_SERVICE:MPCmd::CONTEXT;

    //Create context
    jobs->prepend(new MPCommandJob(this, cmdAddCtx,
                  sdata,
                  [this, jobs, service](const QByteArray &data, bool &) -> bool
    {
        if (pMesProt->getFirstPayloadByte(data) != 1)
        {
            qWarning() << "Failed to add new context";
            jobs->setCurrentJobError("add_context failed on device");
            return false;
        }
        qDebug() << "context " << service << " added";
        return true;
    }));

    //choose context
    jobs->insertAfter(new MPCommandJob(this, cmdSelectCtx,
                                  sdata,
                                  [this, jobs, service](const QByteArray &data, bool &) -> bool
    {
        if (pMesProt->getFirstPayloadByte(data) != 1)
        {
            qWarning() << "Failed to select new context";
            jobs->setCurrentJobError("unable to selected context on device");
            return false;
        }
        qDebug() << "set_context " << service;
        return true;
    }), 0);
}

void MPDevice::setCredential(QString service, const QString &login,
                             const QString &pass, const QString &description, bool setDesc,
                             MessageHandlerCb cb)
{
    if (service.isEmpty())
    {
        qWarning() << "service is empty.";
        cb(false, "service is empty");
        return;
    }

    //Force all service names to lowercase
    service = service.toLower();

    QString logInf = QStringLiteral("Adding/Changing credential for service: %1 login: %2")
                     .arg(service)
                     .arg(login);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    QByteArray sdata = pMesProt->toByteArray(service);
    sdata.append((char)0);

    //First query if context exist
    jobs->append(new MPCommandJob(this, MPCmd::CONTEXT,
                                  sdata,
                                  [this, jobs, service](const QByteArray &data, bool &) -> bool
    {
        if (pMesProt->getFirstPayloadByte(data) != 1)
        {
            qWarning() << "context " << service << " does not exist";
            //Context does not exists, create it
            createJobAddContext(service, jobs);
        }
        else
            qDebug() << "set_context " << service;
        return true;
    }));

    QByteArray ldata = pMesProt->toByteArray(login);
    ldata.append((char)0);

    jobs->append(new MPCommandJob(this, MPCmd::SET_LOGIN,
                                  ldata,
                                  [this, jobs, login](const QByteArray &data, bool &) -> bool
    {
        if (pMesProt->getFirstPayloadByte(data) == 0)
        {
            jobs->setCurrentJobError("set_login failed on device");
            qWarning() << "failed to set login to " << login;
            return false;
        }
        qDebug() << "set_login " << login;
        return true;
    }));

    if (isFw12() && setDesc && description != "None")
    {
        QByteArray ddata = pMesProt->toByteArray(description);
        ddata.append((char)0);

        //Set description should be done right after set login
        jobs->append(new MPCommandJob(this, MPCmd::SET_DESCRIPTION,
                                      ddata,
                                      [this, jobs, description](const QByteArray &data, bool &) -> bool
        {
            if (pMesProt->getFirstPayloadByte(data) == 0)
            {
                if (description.size() > MOOLTIPASS_DESC_SIZE)
                {
                    qWarning() << "description text is more that " << MOOLTIPASS_DESC_SIZE << " chars";
                    jobs->setCurrentJobError(QString("set_description failed on device, max text length allowed is %1 characters").arg(MOOLTIPASS_DESC_SIZE));
                }
                else
                {
                    jobs->setCurrentJobError("set_description failed on device");
                }
                qWarning() << "Failed to set description to: " << description;
                return false;
            }
            qDebug() << "set_description " << description;
            return true;
        }));
    }

    QByteArray pdata = pMesProt->toByteArray(pass);
    pdata.append((char)0);

    if(!pass.isEmpty()) {
        jobs->append(new MPCommandJob(this, MPCmd::CHECK_PASSWORD,
                                      pdata,
                                      [this, jobs, pdata](const QByteArray &data, bool &) -> bool
        {
            if (pMesProt->getFirstPayloadByte(data) != 1)
            {
                //Password does not match, update it
                jobs->prepend(new MPCommandJob(this, MPCmd::SET_PASSWORD,
                                               pdata,
                                               [this, jobs](const QByteArray &data, bool &) -> bool
                {
                    if (pMesProt->getFirstPayloadByte(data) == 0)
                    {
                        jobs->setCurrentJobError("set_password failed on device");
                        qWarning() << "failed to set_password";
                        return false;
                    }
                    qDebug() << "set_password ok";
                    return true;
                }));
            }
            else
                qDebug() << "password not changed";

            return true;
        }));
    }

    connect(jobs, &AsyncJobs::finished, [this, cb](const QByteArray &)
    {
        //all jobs finished success
        qInfo() << "set_credential success";
        cb(true, QString());

        // request change numbers in case they changed
        if (isFw12())
        {
            getChangeNumbers();
        }
        else
        {
            set_credentialsDbChangeNumber(0);
            credentialsDbChangeNumberClone = 0;
            set_dataDbChangeNumber(0);
            dataDbChangeNumberClone = 0;
        }
    });

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        qCritical() << "Failed adding new credential";
        cb(false, failedJob->getErrorStr());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

bool MPDevice::getDataNodeCb(AsyncJobs *jobs,
                             const MPDeviceProgressCb &cbProgress,
                             const QByteArray &data, bool &)
{
    //qDebug() << "getDataNodeCb data size: " << ((quint8)data[0]) << "  " << ((quint8)data[1]) << "  " << ((quint8)data[2]);

    if (pMesProt->getMessageSize(data) == 1 && //data size is 1
        pMesProt->getFirstPayloadByte(data) == 0)   //value is 0 means end of data
    {
        QVariantMap m = jobs->user_data.toMap();
        if (!m.contains("data"))
        {
            //if no data at all, report an error
            jobs->setCurrentJobError("reading data failed or no data");
            return false;
        }
        return true;
    }

    if (pMesProt->getMessageSize(data) != 0)
    {
        QVariantMap m = jobs->user_data.toMap();
        QByteArray ba = m["data"].toByteArray();

        //first packet, we can read the file size
        if (ba.isEmpty())
        {
            ba.append(pMesProt->getFullPayload(data));
            quint32 sz = qFromBigEndian<quint32>((quint8 *)ba.data());
            m["progress_total"] = sz;
            // TODO: send a more significative message
            QVariantMap data = {
                {"total", (int)sz},
                {"current", ba.size() - 4},
                {"msg", "WORKING on getDataNodeCb" }
            };
            cbProgress(data);
        }
        else
        {
            ba.append(pMesProt->getFullPayload(data));
            // TODO: send a more significative message
            QVariantMap data = {
                {"total", m["progress_total"].toInt()},
                {"current", ba.size() - 4},
                {"msg", "WORKING on getDataNodeCb" }
            };
            cbProgress(data);
        }

        m["data"] = ba;
        jobs->user_data = m;

        //ask for the next 32bytes packet
        //bind to a member function of MPDevice, to be able to loop over until with got all the data
        jobs->append(new MPCommandJob(this, MPCmd::READ_DATA_FILE,
                  [this, jobs, cbProgress](const QByteArray &data, bool &done)
                    {
                        return getDataNodeCb(jobs, std::move(cbProgress), data, done);
                    }
                  ));
    }
    return true;
}

void MPDevice::getDataNode(QString service, const QString &fallback_service, const QString &reqid,
                           std::function<void(bool success, QString errstr, QString serv, QByteArray rawData)> cb,
                           const MPDeviceProgressCb &cbProgress)
{
    if (service.isEmpty())
    {
        qWarning() << "context is empty.";
        cb(false, "context is empty", QString(), QByteArray());
        return;
    }

    QString logInf = QStringLiteral("Ask for data node for service: %1 fallback_service: %2 reqid: %3")
                     .arg(service)
                     .arg(fallback_service)
                     .arg(reqid);

    AsyncJobs *jobs;
    if (reqid.isEmpty())
        jobs = new AsyncJobs(logInf, this);
    else
        jobs = new AsyncJobs(logInf, reqid, this);

    QByteArray sdata = pMesProt->toByteArray(service);
    sdata.append((char)0);

    if (isBLE())
    {
        sdata.append((char)0);
        QVariantMap m = {{ "service", service }};
        jobs->user_data = m;
        jobs->append(new MPCommandJob(this, MPCmd::READ_DATA_FILE,
                                      sdata,
                  [this, jobs, cbProgress](const QByteArray &data, bool &)
                    {
                        return bleImpl->readDataNode(jobs, data);
                    }
                  ));
    }
    else
    {
        jobs->append(new MPCommandJob(this, MPCmd::SET_DATA_SERVICE,
                                      sdata,
                                      [this, jobs, service,fallback_service](const QByteArray &data, bool &) -> bool
        {
            if (pMesProt->getFirstPayloadByte(data) != MSG_SUCCESS)
            {
                if (!fallback_service.isEmpty())
                {
                    QByteArray fsdata = pMesProt->toByteArray(fallback_service);
                    fsdata.append((char)0);
                    jobs->prepend(new MPCommandJob(this, MPCmd::SET_DATA_SERVICE,
                                                  fsdata,
                                                  [this, jobs, fallback_service](const QByteArray &data, bool &) -> bool
                    {
                        if (pMesProt->getFirstPayloadByte(data) != 1)
                        {
                            qWarning() << "Error setting context: " << pMesProt->getFirstPayloadByte(data);
                            jobs->setCurrentJobError("failed to select context and fallback_context on device");
                            return false;
                        }

                        QVariantMap m = {{ "service", fallback_service }};
                        jobs->user_data = m;

                        return true;
                    }));
                    return true;
                }

                qWarning() << "Error setting context: " << pMesProt->getFirstPayloadByte(data);
                jobs->setCurrentJobError("failed to select context on device");
                return false;
            }

            QVariantMap m = {{ "service", service }};
            jobs->user_data = m;

            return true;
        }));

        //ask for the first 32bytes packet
        //bind to a member function of MPDevice, to be able to loop over until with got all the data
        jobs->append(new MPCommandJob(this, MPCmd::READ_DATA_FILE,
                  [this, jobs, cbProgress](const QByteArray &data, bool &done)
                    {
                        return getDataNodeCb(jobs, std::move(cbProgress), data, done);
                    }
                  ));
    }

    connect(jobs, &AsyncJobs::finished, [this, jobs, cb](const QByteArray &)
    {
        //all jobs finished success
        qInfo() << "get_data_node success";
        QVariantMap m = jobs->user_data.toMap();
        QByteArray ndata = m["data"].toByteArray();

        if (!isBLE())
        {
            //check data size
            quint32 sz = qFromBigEndian<quint32>((quint8 *)ndata.data());
            qDebug() << "Data size: " << sz;
            ndata = ndata.mid(4, sz);
        }

        cb(true, QString(), m["service"].toString(), ndata);
    });

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        qCritical() << "Failed getting data node";
        cb(false, failedJob->getErrorStr(), QString(), QByteArray());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

bool MPDevice::setDataNodeCb(AsyncJobs *jobs, int current,
                             const MPDeviceProgressCb &cbProgress,
                             const QByteArray &data, bool &)
{
    qDebug() << "setDataNodeCb data current: " << current;

    if (pMesProt->getFirstPayloadByte(data) == 0)
    {
        jobs->setCurrentJobError("writing data to device failed");
        return false;
    }

    //sending finished
    if (current >= currentDataNode.size())
        return true;

    //prepare next block of data
    char eod = (currentDataNode.size() - current <= MOOLTIPASS_BLOCK_SIZE)?1:0;

    QByteArray packet;
    packet.append(eod);
    packet.append(currentDataNode.mid(current, MOOLTIPASS_BLOCK_SIZE));
    packet.resize(MOOLTIPASS_BLOCK_SIZE + 1);

    // TODO: Send more significative message
    QVariantMap cbData = {
        {"total", currentDataNode.size() - MP_DATA_HEADER_SIZE},
        {"current", current + MOOLTIPASS_BLOCK_SIZE},
        {"msg", "WORKING on setDataNodeCb"}
    };
    cbProgress(cbData);

    //send 32bytes packet
    //bind to a member function of MPDevice, to be able to loop over until with got all the data
    jobs->append(new MPCommandJob(this, MPCmd::WRITE_DATA_FILE,
              packet,
              [this, jobs, current, cbProgress](const QByteArray &data, bool &done)
                {
                    return setDataNodeCb(jobs, current + MOOLTIPASS_BLOCK_SIZE, std::move(cbProgress), data, done);
                }
              ));

    return true;
}

void MPDevice::setDataNode(QString service, const QByteArray &nodeData,
                           MessageHandlerCb cb,
                           const MPDeviceProgressCb &cbProgress)
{
    if (service.isEmpty())
    {
        qWarning() << "context is empty.";
        cb(false, "context is empty");
        return;
    }

    //Force all service names to lowercase
    service = service.toLower();

    QString logInf = QStringLiteral("Set data node for service: %1")
                     .arg(service);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    QByteArray sdata = pMesProt->toByteArray(service);
    sdata.append((char)0);

    if (isBLE())
    {
        sdata.append((char)0);
        jobs->append(new MPCommandJob(this, MPCmd::ADD_DATA_SERVICE,
                                      sdata,
                                      [this, service, cb, sdata, jobs](const QByteArray &data, bool &) -> bool
        {
            if (pMesProt->getFirstPayloadByte(data) != MSG_SUCCESS)
            {
                qWarning() << service << " already exists";
                jobs->prepend(new MPCommandJob(this, MPCmd::MODIFY_DATA_FILE, sdata, [this, service](const QByteArray &data, bool&)
                {
                    if (pMesProt->getFirstPayloadByte(data) != MSG_SUCCESS)
                    {
                        qWarning() << "Modify data file failed for: " << service ;
                    }
                    return true;
                }));
            }
            return true;
        }));
    }
    else
    {
        jobs->append(new MPCommandJob(this, MPCmd::SET_DATA_SERVICE,
                                      sdata,
                                      [this, jobs, service](const QByteArray &data, bool &) -> bool
        {
            if (pMesProt->getFirstPayloadByte(data) != 1)
            {
                qWarning() << "context " << service << " does not exist";
                //Context does not exists, create it
                createJobAddContext(service, jobs, true);
            }
            else
                qDebug() << "set_data_context " << service;
            return true;
        }));
    }

    //set size of data
    currentDataNode = QByteArray();
    if (!isBLE())
    {
        currentDataNode.resize(MP_DATA_HEADER_SIZE);
        qToBigEndian(nodeData.size(), (quint8 *)currentDataNode.data());
    }
    currentDataNode.append(nodeData);

    if (isBLE())
    {
        bleImpl->storeFileData(0, jobs, cbProgress);
    }
    else
    {
        //first packet
        QByteArray firstPacket;
        char eod = (nodeData.size() + MP_DATA_HEADER_SIZE <= MOOLTIPASS_BLOCK_SIZE)?1:0;
        firstPacket.append(eod);
        firstPacket.append(currentDataNode.mid(0, MOOLTIPASS_BLOCK_SIZE));
        firstPacket.resize(MOOLTIPASS_BLOCK_SIZE + 1);

        //cbProgress(currentDataNode.size() - MP_DATA_HEADER_SIZE, MOOLTIPASS_BLOCK_SIZE);

        //send the first 32bytes packet
        //bind to a member function of MPDevice, to be able to loop over until with got all the data
        jobs->append(new MPCommandJob(this, MPCmd::WRITE_DATA_FILE,
                  firstPacket,
                  [this, jobs, cbProgress](const QByteArray &data, bool &done)
                    {
                        return setDataNodeCb(jobs, MOOLTIPASS_BLOCK_SIZE, std::move(cbProgress), data, done);
                    }
                  ));
    }

    connect(jobs, &AsyncJobs::finished, [this, cb, service, nodeData](const QByteArray &)
    {
        //all jobs finished success
        qInfo() << "set_data_node success";
        cb(true, QString());

        // update file cache
        addFileToCache(service, ((nodeData.size()+MP_DATA_HEADER_SIZE+MOOLTIPASS_BLOCK_SIZE-1)/MOOLTIPASS_BLOCK_SIZE)*MOOLTIPASS_BLOCK_SIZE);

        // request change numbers in case they changed
        if (isFw12())
        {
            getChangeNumbers();
        }
        else
        {
            set_credentialsDbChangeNumber(0);
            credentialsDbChangeNumberClone = 0;
            set_dataDbChangeNumber(0);
            dataDbChangeNumberClone = 0;

            emit dbChangeNumbersChanged(get_credentialsDbChangeNumber(), get_dataDbChangeNumber());
        }
    });

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        qCritical() << "Failed writing data node";
        cb(false, failedJob->getErrorStr());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void  MPDevice::deleteDataNodesAndLeave(QStringList services,
                                        MessageHandlerCb cb,
                                        const MPDeviceProgressCb &cbProgress)
{
    // TODO for the future:
    // When scanning the parent nodes, store their file sizes
    // Use this file size to make a progress bar for rescan and delete
    // ... and of course to display stats in file MMM

    if (services.isEmpty())
    {
        //No data services to delete, just exit mmm
        exitMemMgmtMode(true);
        return;
    }

    AsyncJobs *jobs = new AsyncJobs("Re-scanning the memory", this);

    /* Re-scan the memory to take into account new changes that may have occured */
    cleanMMMVars();
    memMgmtModeReadFlash(jobs, false, cbProgress, false, true, true);

    connect(jobs, &AsyncJobs::finished, [this, services, cb, cbProgress](const QByteArray &data)
    {
        Q_UNUSED(data);

        for (qint32 i = 0; i < services.size(); i++)
        {
            qDebug() << "Deleting file for " << services[i];
            MPNode* parentPt = findNodeWithNameInList(dataNodes, services[i], true);

            if(!parentPt)
            {
                exitMemMgmtMode(true);
                qCritical() << "Couldn't find node for " << services[i];
                cb(false, "Moolticute Internal Error (DDNAL#1)");
            }
            else
            {
                /* Delete all the childs */
                deleteDataParentChilds(parentPt);

                /* Delete empty parent */
                removeEmptyParentFromDB(parentPt, true);
            }
        }

        /* Check our DB */
        if(!checkLoadedNodes(false, true, false))
        {
            exitMemMgmtMode(true);
            qCritical() << "Error in our internal algo";
            cb(false, "Moolticute Internal Error (DDNAL#2)");
        }

        /* Generate save packets */
        AsyncJobs* saveJobs = new AsyncJobs("Starting save operations...", this);
        connect(saveJobs, &AsyncJobs::finished, [this, services, cb](const QByteArray &data)
        {
            Q_UNUSED(data);
            exitMemMgmtMode(true);
            qInfo() << "Save operations succeeded!";
            cb(true, "Successfully Saved File Database");

            /* Update file cache */
            for (qint32 i = 0; i < services.size(); i++)
            {
                /// Improvement: only trigger file storage after we have removed all files
                removeFileFromCache(services[i]);
            }

            return;
        });
        connect(saveJobs, &AsyncJobs::failed, [this, cb](AsyncJob *failedJob)
        {
            Q_UNUSED(failedJob);
            exitMemMgmtMode(true);
            qCritical() << "Save operations failed!";
            cb(false, "Couldn't Save File Database: Device Unplugged?");
            return;
        });
        if (generateSavePackets(saveJobs, false, true, cbProgress))
        {
            /* Increment db change number */
            if (services.size() > 0 && (isFw12() || isBLE()))
            {
                set_dataDbChangeNumber(get_dataDbChangeNumber() + 1);
                dataDbChangeNumberClone = get_dataDbChangeNumber();
                filesCache.setDbChangeNumber(get_dataDbChangeNumber());
                updateChangeNumbers(saveJobs, Common::DataNumberChanged);
                emit dbChangeNumbersChanged(get_credentialsDbChangeNumber(), get_dataDbChangeNumber());
            }

            /* Run jobs */
            jobsQueue.enqueue(saveJobs);
            runAndDequeueJobs();
        }
        else
        {
            exitMemMgmtMode(true);
            qInfo() << "No changes to make on database";
            cb(true, "No Changes Were Required On Local DB!");
            return;
        }
    });

    connect(jobs, &AsyncJobs::failed, [this, cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Couldn't Rescan The Memory";
        exitMemMgmtMode(true);
        cb(false, "Couldn't Rescan The Memory");
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::changeVirtualAddressesToFreeAddresses(bool onlyChangePwd /* = false*/)
{
    if (virtualStartNode[Common::CRED_ADDR_IDX] != 0)
    {
        qDebug() << "Setting start node to " << getFreeAddress(virtualStartNode[Common::CRED_ADDR_IDX]).toHex();
        startNode[Common::CRED_ADDR_IDX] = getFreeAddress(virtualStartNode[Common::CRED_ADDR_IDX]);
    }
    if (isBLE() && virtualStartNode[Common::WEBAUTHN_ADDR_IDX] != 0)
    {
        qDebug() << "Setting start webauthn node to " << getFreeAddress(virtualStartNode[Common::WEBAUTHN_ADDR_IDX]).toHex();
        startNode[Common::WEBAUTHN_ADDR_IDX] = getFreeAddress(virtualStartNode[Common::WEBAUTHN_ADDR_IDX]);
    }
    if (virtualDataStartNode != 0)
    {
        qDebug() << "Setting data start node to " << getFreeAddress(virtualDataStartNode).toHex();
        startDataNode = getFreeAddress(virtualDataStartNode);
    }
    qDebug() << "Replacing virtual addresses for login nodes...";
    for (auto &i: loginNodes)
    {
        if (i->getAddress().isNull()) i->setAddress(getFreeAddress(i->getVirtualAddress()));
        if (i->getNextParentAddress().isNull()) i->setNextParentAddress(getFreeAddress(i->getNextParentVirtualAddress()));
        if (i->getPreviousParentAddress().isNull()) i->setPreviousParentAddress(getFreeAddress(i->getPreviousParentVirtualAddress()));
        if (i->getStartChildAddress().isNull()) i->setStartChildAddress(getFreeAddress(i->getStartChildVirtualAddress()));
    }
    qDebug() << "Replacing virtual addresses for child nodes...";
    for (auto &i: loginChildNodes)
    {
        int virtAddr = i->getVirtualAddress();
        if (i->getAddress().isNull()) i->setAddress(getFreeAddress(i->getVirtualAddress()));
        if (onlyChangePwd && mmmPasswordChangeNewAddrArray.contains(virtAddr))
        {
            auto& change = mmmPasswordChangeNewAddrArray[virtAddr];
            change.addr = i->getAddress();
        }
        if (i->getNextChildAddress().isNull()) i->setNextChildAddress(getFreeAddress(i->getNextChildVirtualAddress()));
        if (i->getPreviousChildAddress().isNull()) i->setPreviousChildAddress(getFreeAddress(i->getPreviousChildVirtualAddress()));
    }
    qDebug() << "Replacing virtual addresses for data nodes...";
    for (auto &i: dataNodes)
    {
        if (i->getAddress().isNull()) i->setAddress(getFreeAddress(i->getVirtualAddress()));
        if (i->getNextParentAddress().isNull()) i->setNextParentAddress(getFreeAddress(i->getNextParentVirtualAddress()));
        if (i->getPreviousParentAddress().isNull()) i->setPreviousParentAddress(getFreeAddress(i->getPreviousParentVirtualAddress()));
        if (i->getStartChildAddress().isNull()) i->setStartChildAddress(getFreeAddress(i->getStartChildVirtualAddress()));
    }
    qDebug() << "Replacing virtual addresses for data child nodes...";
    for (auto &i: dataChildNodes)
    {
        if (i->getAddress().isNull()) i->setAddress(getFreeAddress(i->getVirtualAddress()));
        if (i->getNextChildDataAddress().isNull()) i->setNextChildDataAddress(getFreeAddress(i->getNextChildVirtualAddress()));
    }

    if (isBLE())
    {
        qDebug() << "Replacing virtual addresses for webauthn login nodes...";
        for (auto &i: webAuthnLoginNodes)
        {
            if (i->getAddress().isNull()) i->setAddress(getFreeAddress(i->getVirtualAddress()));
            if (i->getNextParentAddress().isNull()) i->setNextParentAddress(getFreeAddress(i->getNextParentVirtualAddress()));
            if (i->getPreviousParentAddress().isNull()) i->setPreviousParentAddress(getFreeAddress(i->getPreviousParentVirtualAddress()));
            if (i->getStartChildAddress().isNull()) i->setStartChildAddress(getFreeAddress(i->getStartChildVirtualAddress()));
        }
        qDebug() << "Replacing virtual addresses for webauthn child nodes...";
        for (auto &i: webAuthnLoginChildNodes)
        {
            if (i->getAddress().isNull()) i->setAddress(getFreeAddress(i->getVirtualAddress()));
            if (i->getNextChildAddress().isNull()) i->setNextChildAddress(getFreeAddress(i->getNextChildVirtualAddress()));
            if (i->getPreviousChildAddress().isNull()) i->setPreviousChildAddress(getFreeAddress(i->getPreviousChildVirtualAddress()));
        }
    }
}

void MPDevice::updateChangeNumbers(AsyncJobs *jobs, quint8 flags)
{
    if (isBLE())
    {
        bleImpl->updateChangeNumbers(jobs, flags);
        return;
    }

    QByteArray updateChangeNumbersPacket = QByteArray{};
    updateChangeNumbersPacket.append(get_credentialsDbChangeNumber());
    updateChangeNumbersPacket.append(get_dataDbChangeNumber());
    jobs->append(new MPCommandJob(this, MPCmd::SET_USER_CHANGE_NB, updateChangeNumbersPacket, pMesProt->getDefaultFuncDone()));
}

quint64 MPDevice::getUInt64EncryptionKey(const QString &encryption)
{
    if (Common::SIMPLE_CRYPT_V2 == encryption)
    {
        return getUInt64EncryptionKey();
    }

    return getUInt64EncryptionKeyOld();
}

quint64 MPDevice::getUInt64EncryptionKey()
{
    quint64 key = 0;
    for (int i = 0; i < std::min(8, m_cardCPZ.size()) ; i ++)
    {
        key += ((static_cast<quint64>(m_cardCPZ[i]) & 0xFF) << (i*8));
    }

    return key;
}

quint64 MPDevice::getUInt64EncryptionKeyOld()
{
    qint64 key = 0;
    for (int i = 0; i < std::min(8, m_cardCPZ.size()) ; i ++)
    {
        key += (static_cast<unsigned int>(m_cardCPZ[i]) & 0xFF) << (i*8);
    }

    return key;
}

QString MPDevice::encryptSimpleCrypt(const QByteArray &data, const QString &encryption)
{
    /* Encrypt payload */
    SimpleCrypt simpleCrypt;
    simpleCrypt.setKey(getUInt64EncryptionKey(encryption));

    return simpleCrypt.encryptToString(data);
}

QByteArray MPDevice::decryptSimpleCrypt(const QString &payload, const QString &encryption)
{
    SimpleCrypt simpleCrypt;
    simpleCrypt.setKey(getUInt64EncryptionKey(encryption));

    return simpleCrypt.decryptToByteArray(payload);
}

bool MPDevice::testCodeAgainstCleanDBChanges(AsyncJobs *jobs)
{
    /* Sort the parent list alphabetically */
    std::sort(loginNodes.begin(), loginNodes.end(), [](const MPNode* a, const MPNode* b) -> bool { return a->getService() < b->getService();});
    std::sort(dataNodes.begin(), dataNodes.end(), [](const MPNode* a, const MPNode* b) -> bool { return a->getService() < b->getService();});

    /* Requirements for this code to run: at least 10 credentials, 10 data services, and the first credential to be "_recovered_" with exactly 3 credentials */
    /* The second credential should only have one login */

    QByteArray invalidAddress = QByteArray::fromHex("0200");    // Invalid because in the graphics zone
    MPNode* temp_node_pt;
    MPNode* temp_cnode;
    MPNode* temp_node;

    auto ignoreProgressCb = [](QVariantMap){ };
    qInfo() << "testCodeAgainstCleanDBChanges called, performing tests on our correction algo...";
    qInfo() << "Starting with parent nodes changes...";

    qInfo() << "testCodeAgainstCleanDBChanges: Skipping one parent node link in chain...";
    loginNodes[1]->setNextParentAddress(loginNodes[3]->getAddress());
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Skipping one parent node link in chain: test failed!";return false;} else qInfo() << "Skipping one parent node link in chain: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Skipping first parent node";
    startNode[Common::CRED_ADDR_IDX] = loginNodes[1]->getAddress();
    loginNodes[1]->setPreviousParentAddress(MPNode::EmptyAddress);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Skipping first parent node: test failed!";return false;} else qInfo() << "Skipping first parent node: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Skipping last parent node";
    loginNodes[loginNodes.size()-2]->setNextParentAddress(MPNode::EmptyAddress);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Skipping last parent node: test failed!";return false;} else qInfo() << "Skipping last parent node: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Setting invalid startNode";
    startNode[Common::CRED_ADDR_IDX] = invalidAddress;
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Setting invalid startNode: test failed!";return false;} else qInfo() << "Setting invalid startNode: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Setting parent node loop";
    loginNodes[5]->setPreviousParentAddress(loginNodes[2]->getAddress());
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Setting parent node loop: test failed!";return false;} else qInfo() << "Setting parent node loop: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Breaking linked list";
    loginNodes[5]->setPreviousParentAddress(invalidAddress);
    loginNodes[5]->setNextParentAddress(invalidAddress);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Breaking parent linked list: test failed!";return false;} else qInfo() << "Breaking parent linked list: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Changing valid address for virtual address";
    freeAddresses.append(QByteArray());
    freeAddresses.append(loginNodes[1]->getAddress());
    loginNodes[1]->setAddress(QByteArray(), 1);
    loginNodes[0]->setNextParentAddress(QByteArray(), 1);
    loginNodes[2]->setPreviousParentAddress(QByteArray(), 1);
    changeVirtualAddressesToFreeAddresses();
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Changing valid address for virtual address: test failed!";return false;} else qInfo() << "Changing valid address for virtual address: passed!";

    qInfo() << "Parent node corruption tests passed...";
    qInfo() << "Starting data parent nodes changes...";

    qInfo() << "testCodeAgainstCleanDBChanges: Skipping one data parent node link in chain...";
    dataNodes[1]->setNextParentAddress(dataNodes[3]->getAddress());
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Skipping one data parent node link in chain: test failed!";return false;} else qInfo() << "Skipping one data parent node link in chain: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Skipping first data parent node";
    startDataNode = dataNodes[1]->getAddress();
    dataNodes[1]->setPreviousParentAddress(MPNode::EmptyAddress);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Skipping first data parent node: test failed!";return false;} else qInfo() << "Skipping first data parent node: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Skipping last data parent node";
    dataNodes[dataNodes.size()-2]->setNextParentAddress(MPNode::EmptyAddress);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Skipping last data parent node: test failed!";return false;} else qInfo() << "Skipping last data parent node: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Setting invalid startNode";
    startDataNode = invalidAddress;
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Setting invalid data startNode: test failed!";return false;} else qInfo() << "Setting invalid data startNode: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Setting data parent node loop";
    dataNodes[5]->setPreviousParentAddress(dataNodes[2]->getAddress());
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Setting data parent node loop: test failed!";return false;} else qInfo() << "Setting data parent node loop: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Breaking data parent linked list";
    dataNodes[5]->setPreviousParentAddress(invalidAddress);
    dataNodes[5]->setNextParentAddress(invalidAddress);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Breaking data parent linked list: test failed!";return false;} else qInfo() << "Breaking data parent linked list: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Changing valid address for virtual address";
    freeAddresses.clear();
    freeAddresses.append(QByteArray());
    freeAddresses.append(dataNodes[1]->getAddress());
    dataNodes[1]->setAddress(QByteArray(), 1);
    dataNodes[0]->setNextParentAddress(QByteArray(), 1);
    dataNodes[2]->setPreviousParentAddress(QByteArray(), 1);
    changeVirtualAddressesToFreeAddresses();
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Changing valid address for virtual address: test failed!";return false;} else qInfo() << "Changing valid address for virtual address: passed!";

    qInfo() << "Starting child node corruption tests";

    qInfo() << "testCodeAgainstCleanDBChanges: Creating orphan nodes";
    loginNodes[0]->setStartChildAddress(MPNode::EmptyAddress);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Creating orphan nodes: test failed!";return false;} else qInfo() << "Creating orphan nodes: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Deleting first child node and adding it again";
    temp_node_pt = findNodeWithAddressInList(loginChildNodes, loginNodes[0]->getStartChildAddress(), loginNodes[0]->getStartChildVirtualAddress());
    temp_node = pMesProt->createMPNode(temp_node_pt->getNodeData(), this, temp_node_pt->getAddress(), temp_node_pt->getVirtualAddress());
    removeChildFromDB(loginNodes[0], temp_node_pt, true, true);
    addChildToDB(loginNodes[0], temp_node);
    loginChildNodes.append(temp_node);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Removing & Adding First Child Node: test failed!";return false;} else qInfo() << "Removing & Adding First Child Node: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Deleting middle child node and adding it again";
    temp_node_pt = findNodeWithAddressInList(loginChildNodes, loginNodes[0]->getStartChildAddress(), loginNodes[0]->getStartChildVirtualAddress());
    temp_node_pt = findNodeWithAddressInList(loginChildNodes, temp_node_pt->getNextChildAddress(), temp_node_pt->getNextChildVirtualAddress());
    temp_node = pMesProt->createMPNode(temp_node_pt->getNodeData(), this, temp_node_pt->getAddress(), temp_node_pt->getVirtualAddress());
    removeChildFromDB(loginNodes[0], temp_node_pt, true, true);
    addChildToDB(loginNodes[0], temp_node);
    loginChildNodes.append(temp_node);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Removing & Adding Middle Child Node: test failed!";return false;} else qInfo() << "Removing & Adding Middle Child Node: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Deleting last child node and adding it again";
    temp_node_pt = findNodeWithAddressInList(loginChildNodes, loginNodes[0]->getStartChildAddress(), loginNodes[0]->getStartChildVirtualAddress());
    temp_node_pt = findNodeWithAddressInList(loginChildNodes, temp_node_pt->getNextChildAddress(), temp_node_pt->getNextChildVirtualAddress());
    temp_node_pt = findNodeWithAddressInList(loginChildNodes, temp_node_pt->getNextChildAddress(), temp_node_pt->getNextChildVirtualAddress());
    temp_node = pMesProt->createMPNode(temp_node_pt->getNodeData(), this, temp_node_pt->getAddress(), temp_node_pt->getVirtualAddress());
    removeChildFromDB(loginNodes[0], temp_node_pt, true, true);
    addChildToDB(loginNodes[0], temp_node);
    loginChildNodes.append(temp_node);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Removing & Adding Last Child Node: test failed!";return false;} else qInfo() << "Removing & Adding Last Child Node: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Deleting parent node with single child node and adding it again";
    temp_node_pt = findNodeWithAddressInList(loginChildNodes, loginNodes[1]->getStartChildAddress(), loginNodes[1]->getStartChildVirtualAddress());
    temp_cnode = pMesProt->createMPNode(temp_node_pt->getNodeData(), this, temp_node_pt->getAddress(), temp_node_pt->getVirtualAddress());
    temp_node = pMesProt->createMPNode(loginNodes[1]->getNodeData(), this, loginNodes[1]->getAddress(), loginNodes[1]->getVirtualAddress());
    temp_node->setStartChildAddress(MPNode::EmptyAddress, 0);
    removeChildFromDB(loginNodes[1], temp_node_pt, true, true);
    loginNodes.append(temp_node);
    addOrphanParentToDB(temp_node, false, true);
    loginChildNodes.append(temp_cnode);
    addChildToDB(temp_node, temp_cnode);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Removing & Adding Single Child Node: test failed!";return false;} else qInfo() << "Removing & Adding Single Child Node: passed!";

    qInfo() << "All tests were successfully run!";
    return true;
}

QByteArray MPDevice::generateExportFileData(const QString &encryption)
{
    QJsonArray exportTopArray = QJsonArray();

    /* CTR */
    exportTopArray.append(QJsonValue(Common::bytesToJsonObjectArray(ctrValue)));

    /* CPZ/CTR packets */
    QJsonArray cpzCtrQJsonArray = QJsonArray();
    for (qint32 i = 0; i < cpzCtrValue.size(); i++)
    {
        cpzCtrQJsonArray.append(QJsonValue(Common::bytesToJsonObjectArray(cpzCtrValue[i])));
    }
    exportTopArray.append(QJsonValue(cpzCtrQJsonArray));

    /* Starting parent */
    exportTopArray.append(QJsonValue(Common::bytesToJson(startNode[Common::CRED_ADDR_IDX])));

    /* Data starting parent */
    exportTopArray.append(QJsonValue(Common::bytesToJson(startDataNode)));

    /* Favorites */
    QJsonArray favQJsonArray = QJsonArray();
    for (qint32 i = 0; i < favoritesAddrs.size(); i++)
    {
        favQJsonArray.append(QJsonValue(Common::bytesToJsonObjectArray(favoritesAddrs[i])));
    }
    exportTopArray.append(QJsonValue(favQJsonArray));

    /* Service nodes */
    QJsonArray nodeQJsonArray = QJsonArray();
    for (qint32 i = 0; i < loginNodes.size(); i++)
    {
        QJsonObject nodeObject = QJsonObject();
        nodeObject["address"] = QJsonValue(Common::bytesToJson(loginNodes[i]->getAddress()));
        nodeObject["name"] = QJsonValue(loginNodes[i]->getService());
        nodeObject["data"] = QJsonValue(Common::bytesToJsonObjectArray(loginNodes[i]->getNodeData()));
        nodeQJsonArray.append(QJsonValue(nodeObject));
    }
    exportTopArray.append(QJsonValue(nodeQJsonArray));

    /* Child nodes */
    nodeQJsonArray = QJsonArray();
    for (qint32 i = 0; i < loginChildNodes.size(); i++)
    {
        QJsonObject nodeObject = QJsonObject();
        nodeObject["address"] = QJsonValue(Common::bytesToJson(loginChildNodes[i]->getAddress()));
        nodeObject["name"] = QJsonValue(loginChildNodes[i]->getLogin());
        nodeObject["data"] = QJsonValue(Common::bytesToJsonObjectArray(loginChildNodes[i]->getNodeData()));
        nodeObject["pointed"] = QJsonValue(false);
        nodeQJsonArray.append(QJsonValue(nodeObject));
    }
    exportTopArray.append(QJsonValue(nodeQJsonArray));

    /* Child nodes */
    nodeQJsonArray = QJsonArray();
    for (qint32 i = 0; i < dataNodes.size(); i++)
    {
        QJsonObject nodeObject = QJsonObject();
        nodeObject["address"] = QJsonValue(Common::bytesToJson(dataNodes[i]->getAddress()));
        nodeObject["name"] = QJsonValue(dataNodes[i]->getService());
        nodeObject["data"] = QJsonValue(Common::bytesToJsonObjectArray(dataNodes[i]->getNodeData()));
        nodeQJsonArray.append(QJsonValue(nodeObject));
    }
    exportTopArray.append(QJsonValue(nodeQJsonArray));

    /* Child nodes */
    nodeQJsonArray = QJsonArray();
    for (qint32 i = 0; i < dataChildNodes.size(); i++)
    {
        QJsonObject nodeObject = QJsonObject();
        nodeObject["address"] = QJsonValue(Common::bytesToJson(dataChildNodes[i]->getAddress()));
        nodeObject["data"] = QJsonValue(Common::bytesToJsonObjectArray(dataChildNodes[i]->getNodeData()));
        nodeObject["pointed"] = QJsonValue(false);
        nodeQJsonArray.append(QJsonValue(nodeObject));
    }
    exportTopArray.append(QJsonValue(nodeQJsonArray));

    /* identifier */
    exportTopArray.append(QJsonValue(QString("moolticute")));

    /* bundle version */
    exportTopArray.append(QJsonValue((qint64)1));

    /* Credential change number */
    exportTopArray.append(QJsonValue((quint8)get_credentialsDbChangeNumber()));

    /* Data change number */
    exportTopArray.append(QJsonValue((quint8)get_dataDbChangeNumber()));

    /* Mooltipass serial */
    exportTopArray.append(QJsonValue((qint64)get_serialNumber()));

    if (isBLE())
    {
        bleImpl->generateExportData(exportTopArray);
    }

    /* Generate file payload */
    QJsonDocument payloadDoc(exportTopArray);
    auto payload = payloadDoc.toJson();

    qDebug() << "requested encryption for exported DB:" << encryption;

    if (encryption.isEmpty() || encryption == "none")
        return payload;

    /* Export file content */
    QJsonObject exportTopObject;

    QString enc = encryption;
    if (enc == Common::SIMPLE_CRYPT || enc == Common::SIMPLE_CRYPT_V2)
    {
        if (isBLE())
        {
            // For BLE using the new encryption
            enc = Common::SIMPLE_CRYPT_V2;
        }
        exportTopObject.insert("encryption", enc);
        exportTopObject.insert("payload", encryptSimpleCrypt(payload, enc));
    } else
    {
        // Fallback in case of an unknown encryption method where specified
        qWarning() << "DB export: Unknown encryption " << enc << "is asked, fallback to 'none'";
        exportTopObject.insert("encryption", "none");
        exportTopObject.insert("payload", QString(payload));
    }

    exportTopObject.insert("dataDbChangeNumber", QJsonValue((quint8)get_dataDbChangeNumber()));
    exportTopObject.insert("credentialsDbChangeNumber", QJsonValue((quint8)get_credentialsDbChangeNumber()));

    QJsonDocument fileContentDoc(exportTopObject);
    payload = fileContentDoc.toJson();

    return payload;
}

bool MPDevice::readExportFile(const QByteArray &fileData, QString &errorString)
{
    /* When we add nodes, we give them an address based on this counter */
    cleanMMMVars();
    cleanImportedVars();

    /* Local vars */
    QJsonObject qjobject;
    QJsonArray qjarray;

    /* Use a qjson document */
    QJsonDocument d = QJsonDocument::fromJson(fileData);

    /* Checks */
    if (d.isEmpty())
    {
        qCritical() << "JSON document is empty";
        errorString = "Selected File Isn't Correct";
        return false;
    }
    else if (d.isNull())
    {
        qCritical() << "JSON document is null";
        errorString = "Selected File Isn't Correct";
        return false;
    }

    if (d.isArray())
    {
        /** Mooltiapp / Chrome App save file **/

        /* Get the array */
        QJsonArray dataArray = d.array();
        return readExportPayload(dataArray, errorString);
    }
    else if (d.isObject())
    {
        /* Use object */
        QJsonObject importFile = d.object();
        qInfo() << importFile.keys();
        if ( importFile.contains("encryption") && importFile.contains("payload") )
        {
            auto encryptionMethod = importFile.value("encryption").toString();
            if ( encryptionMethod == Common::SIMPLE_CRYPT || encryptionMethod == Common::SIMPLE_CRYPT_V2 )
            {
                QString payload = importFile.value("payload").toString();
                auto decryptedData = decryptSimpleCrypt(payload, encryptionMethod);

                QJsonDocument decryptedDocument = QJsonDocument::fromJson(decryptedData);
                if (decryptedDocument.isArray())
                {
                    /* Get the array */
                    QJsonArray dataArray = decryptedDocument.array();
                    return readExportPayload(dataArray, errorString);
                }
                else
                {
                    qCritical() << "Encrypted payload isn't correct";
                    errorString = "Selected File Is Another User's Backup";
                    return false;
                }
            }
            else if  (encryptionMethod == "none")
            {
                /* Legacy, not generated anymore */
                return readExportPayload(QJsonDocument::fromJson(importFile.value("payload").toString().toUtf8()).array(), errorString);
            }
            else
            {
                errorString = "Unknown Encryption Method";
                return false;
            }
        }

        qInfo() << "File is a JSON object";
        errorString = "Selected File Isn't Correct";
        return false;
    }
    else
    {
        /* If it's not an array or an object... */
        errorString = "Selected File Isn't Correct";
        return false;
    }
}

void MPDevice::readExportNodes(QJsonArray &&nodes, ExportPayloadData id, bool fromMiniToBle /*= false*/)
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
        for (qint32 j = 0; j < dataObj.size(); j++) {dataCore.append(dataObj[QString::number(j)].toInt());}
        if (fromMiniToBle)
        {
            bleImpl->convertMiniToBleNode(dataCore);
        }

        /* Recreate node and add it to the list of imported nodes */
        MPNode* importedNode = pMesProt->createMPNode(qMove(dataCore), this, qMove(serviceAddr), 0);
        importNodeMap[id]->append(importedNode);
    }
}

bool MPDevice::readExportPayload(QJsonArray dataArray, QString &errorString)
{
    /** Mooltiapp / Chrome App save file **/

    const auto dataSize = dataArray.size();
    const bool isBleExport = dataSize >= BLE_EXPORT_FIELD_MIN_NUM && dataArray[EXPORT_IS_BLE_INDEX].toBool();
    const bool isMiniExportFile = dataSize == MC_EXPORT_FIELD_NUM;
    const QString deviceVersion = dataArray[EXPORT_DEVICE_VERSION_INDEX].toString();
    /* Checks */
    if (!((deviceVersion == "mooltipass" && dataSize == MP_EXPORT_FIELD_NUM)
          || (deviceVersion == "moolticute" && (isMiniExportFile || isBleExport))))
    {
        qCritical() << "Invalid MooltiApp file";
        errorString = "Selected File Isn't Correct";
        return false;
    }

    if (isBleExport && !isBLE())
    {
        qWarning() << "Cannot use BLE export file for mini";
        errorString = "Selected File Is A BLE Backup";
        return false;
    }

    /* Know which bundle we're dealing with */
    if (deviceVersion == "mooltipass")
    {
        isMooltiAppImportFile = true;
        qInfo() << "Dealing with MooltiApp export file";
    }
    else
    {
        qInfo() << "Dealing with Moolticute export file";
        isMooltiAppImportFile = false;
        importedCredentialsDbChangeNumber = dataArray[EXPORT_CRED_CHANGE_NUMBER_INDEX].toInt();
        qDebug() << "Imported cred change number: " << importedCredentialsDbChangeNumber;
        importedDataDbChangeNumber = dataArray[EXPORT_DATA_CHANGE_NUMBER_INDEX].toInt();
        qDebug() << "Imported data change number: " << importedDataDbChangeNumber;
        importedDbMiniSerialNumber = dataArray[EXPORT_DB_MINI_SERIAL_NUM_INDEX].toInt();
        qDebug() << "Imported mini serial number: " << importedDbMiniSerialNumber;
    }

    const bool miniExportToBle = isBLE() && isMiniExportFile;

    /* Read CTR */
    importedCtrValue = QByteArray();
    auto qjobject = dataArray[EXPORT_CTR_INDEX].toObject();
    for (qint32 i = 0; i < qjobject.size(); i++) {importedCtrValue.append(qjobject[QString::number(i)].toInt());}
    qDebug() << "Imported CTR: " << importedCtrValue.toHex();

    /* Read CPZ CTR values */
    auto qjarray = dataArray[EXPORT_CPZ_CTR_INDEX].toArray();
    for (qint32 i = 0; i < qjarray.size(); i++)
    {
        qjobject = qjarray[i].toObject();
        QByteArray qbarray = QByteArray();
        if (miniExportToBle)
        {
            //Add userid placeholder to BLE cpz lut entry
            bleImpl->addUserIdPlaceholder(qbarray);
        }
        for (qint32 j = 0; j < qjobject.size(); j++) {qbarray.append(qjobject[QString::number(j)].toInt());}
        qDebug() << "Imported CPZ/CTR value : " << qbarray.toHex();
        importedCpzCtrValue.append(qbarray);
    }

    /* Check if one of them is for the current card */
    bool cpzFound = false;
    for (qint32 i = 0; i < importedCpzCtrValue.size(); i++)
    {
        if (pMesProt->getCpzValue(importedCpzCtrValue[i]) == get_cardCPZ())
        {
            qDebug() << "Import file is a backup for current user";
            unknownCardAddPayload = importedCpzCtrValue[i];
            if (miniExportToBle)
            {
                bleImpl->fillMiniExportPayload(unknownCardAddPayload);
            }
            cpzFound = true;
        }
    }

    bool needToAddExistingUser = isBleExport && dataArray.size() >= EXPORT_USB_LAYOUT_INDEX && Common::UnknownSmartcard == get_status();
    //Check export file if contains user datas
    if (needToAddExistingUser)
    {
        cpzFound = true;
    }
    else if (isBleExport)
    {
        // Prevent ADD_UNKNOWN_CARD message
        unknownCardAddPayload.clear();
    }

    if (!cpzFound)
    {
        qWarning() << "Import file is not a backup for current user";
        errorString = "Selected File Is Another User's Backup";
        return false;
    }

    /* Read Starting Parent */
    importedStartNode = QByteArray();
    qjarray = dataArray[EXPORT_STARTING_PARENT_INDEX].toArray();
    for (qint32 i = 0; i < qjarray.size(); i++) {importedStartNode.append(qjarray[i].toInt());}
    qDebug() << "Imported start node: " << importedStartNode.toHex();

    /* Read Data Starting Parent */
    importedStartDataNode = QByteArray();
    qjarray = dataArray[EXPORT_DATA_STARTING_PARENT_INDEX].toArray();
    for (qint32 i = 0; i < qjarray.size(); i++) {importedStartDataNode.append(qjarray[i].toInt());}
    qDebug() << "Imported data start node: " << importedStartDataNode.toHex();

    /* Read favorites */
    qjarray = dataArray[EXPORT_FAVORITES_INDEX].toArray();
    for (qint32 i = 0; i < qjarray.size(); i++)
    {
        qjobject = qjarray[i].toObject();
        QByteArray qbarray = QByteArray();
        for (qint32 j = 0; j < qjobject.size(); j++) {qbarray.append(qjobject[QString::number(j)].toInt());}
        qDebug() << "Imported favorite " << i << " : " << qbarray.toHex();
        importedFavoritesAddrs.append(qbarray);
    }

    /* Read service nodes */
    readExportNodes(dataArray[EXPORT_SERVICE_NODES_INDEX].toArray(), EXPORT_SERVICE_NODES_INDEX, miniExportToBle);

    /* Read service child nodes */
    readExportNodes(dataArray[EXPORT_SERVICE_CHILD_NODES_INDEX].toArray(), EXPORT_SERVICE_CHILD_NODES_INDEX, miniExportToBle);

    if (!isMooltiAppImportFile)
    {
        if (!miniExportToBle)
        {
            /* Read service nodes */
            readExportNodes(dataArray[EXPORT_MC_SERVICE_NODES_INDEX].toArray(), EXPORT_MC_SERVICE_NODES_INDEX);

            /* Read service child nodes */
            readExportNodes(dataArray[EXPORT_MC_SERVICE_CHILD_NODES_INDEX].toArray(), EXPORT_MC_SERVICE_CHILD_NODES_INDEX);
        }

        if (isBleExport)
        {
            /* Read webauthn nodes */
            readExportNodes(dataArray[EXPORT_WEBAUTHN_NODES_INDEX].toArray(), EXPORT_WEBAUTHN_NODES_INDEX);

            /* Read webauthn child nodes */
            readExportNodes(dataArray[EXPORT_WEBAUTHN_CHILD_NODES_INDEX].toArray(), EXPORT_WEBAUTHN_CHILD_NODES_INDEX);

            bleImpl->setImportUserCategories(dataArray[EXPORT_BLE_USER_CATEGORIES_INDEX].toObject());
            if (needToAddExistingUser)
            {
                bleImpl->fillAddUnknownCard(dataArray);
            }
        }
    }

    return true;
}

void MPDevice::cleanImportedVars(void)
{
    virtualStartNode = {0, 0};
    virtualDataStartNode = 0;
    newAddressesNeededCounter = 0;
    importedCtrValue.clear();
    importedStartNode.clear();
    importedStartDataNode.clear();
    importedCpzCtrValue.clear();
    importedFavoritesAddrs.clear();
    qDeleteAll(importedLoginNodes);
    qDeleteAll(importedLoginChildNodes);
    qDeleteAll(importedDataNodes);
    qDeleteAll(importedDataChildNodes);
    qDeleteAll(importedWebauthnLoginNodes);
    qDeleteAll(importedWebauthnLoginChildNodes);
    importedLoginNodes.clear();
    importedLoginChildNodes.clear();
    importedDataNodes.clear();
    importedDataChildNodes.clear();
    importedWebauthnLoginNodes.clear();
    importedWebauthnLoginChildNodes.clear();
}

void MPDevice::cleanMMMVars(void)
{
    /* Cleaning all temp values */
    virtualStartNode = {0, 0};
    virtualDataStartNode = 0;
    ctrValue.clear();
    cpzCtrValue.clear();
    clearAndDelete(loginChildNodes);
    clearAndDelete(dataChildNodes);
    clearAndDelete(loginNodes);
    clearAndDelete(dataNodes);
    favoritesAddrs.clear();
    /* Cleaning the clones as well */
    ctrValueClone.clear();
    cpzCtrValueClone.clear();
    clearAndDelete(loginChildNodesClone);
    clearAndDelete(dataChildNodesClone);
    clearAndDelete(loginNodesClone);
    clearAndDelete(dataNodesClone);
    favoritesAddrsClone.clear();
    freeAddresses.clear();
    if (isBLE())
    {
        clearAndDelete(webAuthnLoginChildNodes);
        clearAndDelete(webAuthnLoginChildNodesClone);
        clearAndDelete(webAuthnLoginNodes);
        clearAndDelete(webAuthnLoginNodesClone);
        bleImpl->getFreeAddressProvider().cleanFreeAddresses();
    }
}

void MPDevice::startImportFileMerging(const MPDeviceProgressCb &cbProgress, MessageHandlerCb cb, bool noDelete)
{
    /* New job for starting MMM */
    AsyncJobs *jobs = new AsyncJobs("Starting MMM mode for import file merging", this);

    /* Ask device to go into MMM first */
    if (!isBLE() || get_status() != Common::UnknownSmartcard)
    {
        jobs->append(new MPCommandJob(this, MPCmd::START_MEMORYMGMT, pMesProt->getDefaultFuncDone()));
    }
    /* Load flash contents the usual way */
    memMgmtModeReadFlash(jobs, false,
                            cbProgress,
                            true, true, true);

    connect(jobs, &AsyncJobs::finished, [this, cb, cbProgress, noDelete](const QByteArray &data)
    {
        Q_UNUSED(data);
        qInfo() << "Mem management mode enabled";

        /* Tag favorites */
        tagFavoriteNodes();

        /* We arrive here knowing that the CPZ is in the CPZ/CTR list */
        qInfo() << "Starting File Merging...";

        if (!isBLE())
        {
            /// Know if we need to add CPZ CTR packets (additive process)
            for (qint32 i = 0; i < importedCpzCtrValue.size(); i++)
            {
                bool cpzFound = false;
                for (qint32 j = 0; j < cpzCtrValue.size(); j++)
                {
                    if (cpzCtrValue[j] == importedCpzCtrValue[i])
                    {
                        cpzFound = true;
                        break;
                    }
                }

                if (!cpzFound)
                {
                    qDebug() << "CPZ CTR not in our DB: " << importedCpzCtrValue[i].toHex();
                    cpzCtrValue.append(importedCpzCtrValue[i]);
                }
            }
        }

        /// Check CTR value
        quint32 cur_ctr = ((quint8)ctrValue[0])*256*256 + ((quint8)ctrValue[1])*256 + ((quint8)ctrValue[2]);
        quint32 imported_ctr = ((quint8)importedCtrValue[0])*256*256 + ((quint8)importedCtrValue[1]*256) + ((quint8)importedCtrValue[2]);
        if (imported_ctr > cur_ctr)
        {
            qDebug() << "CTR value mismatch: " << ctrValue.toHex() << " instead of " << importedCtrValue.toHex();
            ctrValue = QByteArray(importedCtrValue);
        }

        if (!checkImportedLoginNodes(cb, Common::CRED_ADDR_IDX))
        {
            qCritical() << "Login import failed";
            return;
        }

        if (isBLE())
        {
            if (!checkImportedLoginNodes(cb, Common::WEBAUTHN_ADDR_IDX))
            {
                qCritical() << "Login import failed";
                return;
            }
            bleImpl->importUserCategories();
        }

        /// Same but for data nodes
        if (!isMooltiAppImportFile)
        {
            const bool dataImportSuccess = checkImportedDataNodes(cb);
            if (!dataImportSuccess)
            {
                qCritical() << "Data import failed";
                return;
            }
        }

        qInfo() << newAddressesNeededCounter << " addresses are required for merge operations ";

        /* If we need addresses, query them */
        if (newAddressesNeededCounter > 0)
        {
            /* Add one extra address because we do pre-increment on that counter */
            newAddressesNeededCounter++;

            AsyncJobs* getFreeAddressesJob = new AsyncJobs("Asking free adresses...", this);
            loadFreeAddresses(getFreeAddressesJob, MPNode::EmptyAddress, false, cbProgress);

            connect(getFreeAddressesJob, &AsyncJobs::finished, [this, cb, cbProgress, noDelete](const QByteArray &data)
            {
                Q_UNUSED(data);
                //data is last result
                //all jobs finished success
                qInfo() << "Got all required free addresses...";

                /* We got all the addresses, change virtual addrs for real addrs and finish merging */
                changeVirtualAddressesToFreeAddresses();

                /* Finish import file merging */
                QString stringError;
                if(finishImportFileMerging(stringError, noDelete))
                {
                    AsyncJobs* mergeOperations = new AsyncJobs("Starting merge operations...", this);
                    connect(mergeOperations, &AsyncJobs::finished, [this, cb](const QByteArray &data)
                    {
                        Q_UNUSED(data);

                        /* Update file cache */
                        QList<QVariantMap> list;
                        for (auto &i: dataNodes)
                        {
                            QVariantMap item;
                            item.insert("revision", 0);
                            item.insert("name", i->getService());
                            item.insert("size", i->getEncDataSize());
                            list.append(item);
                        }
                        filesCache.save(list);
                        filesCache.setDbChangeNumber(importedDataDbChangeNumber);
                        emit filesCacheChanged();

                        cleanImportedVars();
                        exitMemMgmtMode(false);
                        qInfo() << "Merge operations succeeded!";
                        cb(true, "Import File Merged With Current Database");
                        return;
                    });
                    connect(mergeOperations, &AsyncJobs::failed, [this, cb](AsyncJob *failedJob)
                    {
                        Q_UNUSED(failedJob);
                        cleanImportedVars();
                        exitMemMgmtMode(false);
                        qCritical() << "Merge operations failed!";
                        cb(false, "Couldn't Import Database: Device Unplugged?");
                        return;
                    });
                    if (generateSavePackets(mergeOperations, true, !isMooltiAppImportFile, cbProgress))
                    {
                        jobsQueue.enqueue(mergeOperations);
                        runAndDequeueJobs();
                    }
                    else
                    {
                        cleanImportedVars();
                        exitMemMgmtMode(false);
                        qInfo() << "Databases already synced";
                        cb(true, "Local Database Already Is Up To Date!");
                        return;
                    }
                }
                else
                {
                    cleanImportedVars();
                    exitMemMgmtMode(false);
                    cb(false, "Couldn't Import Database: Corrupted Database File");
                    return;
                }
            });

            connect(getFreeAddressesJob, &AsyncJobs::failed, [](AsyncJob *failedJob)
            {
                Q_UNUSED(failedJob);
                qCritical() << "Couldn't get enough free addresses";
            });

            jobsQueue.enqueue(getFreeAddressesJob);
            runAndDequeueJobs();
        }
        else
        {
            /* Finish import file merging */
            QString stringError;
            if(finishImportFileMerging(stringError, noDelete))
            {
                AsyncJobs* mergeOperations = new AsyncJobs("Starting merge operations...", this);
                connect(mergeOperations, &AsyncJobs::finished, [this, cb](const QByteArray &data)
                {
                    Q_UNUSED(data);

                    /* Update file cache */
                    QList<QVariantMap> list;
                    for (auto &i: dataNodes)
                    {
                        QVariantMap item;
                        item.insert("revision", 0);
                        item.insert("name", i->getService());
                        item.insert("size", i->getEncDataSize());
                        list.append(item);
                    }
                    filesCache.save(list);
                    filesCache.setDbChangeNumber(importedDataDbChangeNumber);
                    emit filesCacheChanged();

                    cleanImportedVars();
                    exitMemMgmtMode(false);
                    qInfo() << "Merge operations succeeded!";
                    cb(true, "Import File Merged With Current Database");
                    return;
                });
                connect(mergeOperations, &AsyncJobs::failed, [this, cb](AsyncJob *failedJob)
                {
                    Q_UNUSED(failedJob);
                    cleanImportedVars();
                    exitMemMgmtMode(false);
                    qCritical() << "Merge operations failed!";
                    cb(false, "Couldn't Import Database: Device Unplugged?");
                    return;
                });
                if (generateSavePackets(mergeOperations, true, !isMooltiAppImportFile, cbProgress))
                {
                    jobsQueue.enqueue(mergeOperations);
                    runAndDequeueJobs();
                }
                else
                {
                    cleanImportedVars();
                    exitMemMgmtMode(false);
                    qInfo() << "Databases already synced";
                    cb(true, "Local Database Already Is Up To Date!");
                    return;
                }
            }
            else
            {
                cleanImportedVars();
                exitMemMgmtMode(false);
                cb(false, "Couldn't Import Database: Corrupted Database File");
                return;
            }
        }
    });

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Setting device in MMM failed";
        cb(false, "Please Retry and Approve Credential Management");
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

bool MPDevice::checkImportedLoginNodes(const MessageHandlerCb &cb, Common::AddressType addrType)
{
    const bool isCred = addrType == Common::CRED_ADDR_IDX;
    NodeList& importNodes = isCred ? importedLoginNodes : importedWebauthnLoginNodes;
    NodeList& importChildNodes = isCred ? importedLoginChildNodes : importedWebauthnLoginChildNodes;
    NodeList& nodes = isCred ? loginNodes : webAuthnLoginNodes;
    NodeList& childNodes = isCred? loginChildNodes : webAuthnLoginChildNodes;
    /// Find the nodes we don't have in memory or that have been changed
    for (qint32 i = 0; i < importNodes.size(); i++)
    {
        bool service_node_found = false;

        // Loop in the memory nodes to compare data
        for (qint32 j = 0; j < nodes.size(); j++)
        {
            if (importNodes[i]->getLoginNodeData() == nodes[j]->getLoginNodeData())
            {
                // We found a parent node that has the same core data (doesn't mean the same prev / next node though!)
                //qDebug() << "Parent node core data match for " << importNodes[i]->getService();
                nodes[j]->setMergeTagged();
                service_node_found = true;

                // Next step is to check if the children are the same
                quint32 cur_import_child_node_addr_v = importNodes[i]->getStartChildVirtualAddress();
                QByteArray cur_import_child_node_addr = importNodes[i]->getStartChildAddress();
                quint32 matched_parent_first_child_v = nodes[j]->getStartChildVirtualAddress();
                QByteArray matched_parent_first_child = nodes[j]->getStartChildAddress();

                /* Special case: parent doesn't have children but we do */
                if (((cur_import_child_node_addr == MPNode::EmptyAddress) || (cur_import_child_node_addr.isNull() && cur_import_child_node_addr_v == 0)) && ((matched_parent_first_child != MPNode::EmptyAddress) || (matched_parent_first_child.isNull() && matched_parent_first_child_v != 0)))
                {
                    nodes[j]->setStartChildAddress(MPNode::EmptyAddress);
                }

                //qDebug() << "First child address for imported node: " << cur_import_child_node_addr.toHex() << " , for own node: " << matched_parent_first_child.toHex();
                while ((cur_import_child_node_addr != MPNode::EmptyAddress) || (cur_import_child_node_addr.isNull() && cur_import_child_node_addr_v != 0))
                {
                    // Find the imported child node in our list
                    MPNode* imported_child_node = findNodeWithAddressInList(importChildNodes, cur_import_child_node_addr, cur_import_child_node_addr_v);

                    // Check if we actually found the node
                    if (!imported_child_node)
                    {
                        cleanImportedVars();
                        exitMemMgmtMode(false);
                        cb(false, "Couldn't Import Database: Corrupted Export File");
                        qCritical() << "Couldn't find imported child node in our list (corrupted import file?)";
                        return false;
                    }

                    // We found the imported child, now we need to find the one that matches
                    //qDebug() << "Looking for child " + imported_child_node->getLogin() + " in the Mooltipass";

                    // Try to find the match between the child nodes of the matched parent
                    bool matched_login_found = false;
                    QByteArray matched_parent_next_child = QByteArray(matched_parent_first_child);
                    quint32 matched_parent_next_child_v = matched_parent_first_child_v;
                    while (((matched_parent_next_child != MPNode::EmptyAddress) || (matched_parent_next_child.isNull() && matched_parent_next_child_v != 0)) && (matched_login_found == false))
                    {
                        // Find the child node at this address
                        MPNode* cur_child_node = findNodeWithAddressInList(childNodes, matched_parent_next_child, matched_parent_next_child_v);

                        if (!cur_child_node)
                        {
                            cleanImportedVars();
                            exitMemMgmtMode(false);
                            cb(false, "Couldn't Import Database: Please Run Integrity Check");
                            qCritical() << "Couldn't find child node in our list (bad node reading?)";
                            return false;
                        }

                        // We found the child, now we can compare the login name
                        if (cur_child_node->getLogin() == imported_child_node->getLogin())
                        {
                            // We have a match between imported login node & current login node
                            //qDebug() << "Child found in the mooltipass, comparing rest of the data";
                            cur_child_node->setMergeTagged();
                            matched_login_found = true;

                            if (cur_child_node->getLoginChildNodeData() == imported_child_node->getLoginChildNodeData())
                            {
                                //qDebug() << importNodes[i]->getService() << " : child core data match for child " << imported_child_node->getLogin() << " , nothing to do";
                            }
                            else
                            {
                                // Data mismatch, overwrite the important part
                                qDebug() << importNodes[i]->getService() << " : child core data mismatch for child " << imported_child_node->getLogin() << " , updating...";
                                cur_child_node->setLoginChildNodeData(imported_child_node->getNodeFlags(), imported_child_node->getLoginChildNodeData());
                            }
                            break;
                        }
                        else
                        {
                            // Not a match, load next child address in var
                            matched_parent_next_child = cur_child_node->getNextChildAddress();
                            matched_parent_next_child_v = cur_child_node->getNextChildVirtualAddress();
                        }
                    }

                    // If we couldn't find the child node, we have to add it
                    if(matched_login_found == false)
                    {
                        qDebug() << importNodes[i]->getService() << " : adding new child " << imported_child_node->getLogin() << " in the mooltipass...";

                        /* Increment new addresses counter */
                        incrementNeededAddresses(MPNode::NodeChild);

                        /* Create new node with null address and virtual address set to our counter value */
                        MPNode* newChildNodePt = pMesProt->createMPNode(QByteArray(getChildNodeSize(), 0), this, QByteArray(), newAddressesNeededCounter);
                        newChildNodePt->setType(MPNode::NodeChild);
                        newChildNodePt->setLoginChildNodeData(imported_child_node->getNodeFlags(), imported_child_node->getLoginChildNodeData());
                        newChildNodePt->setMergeTagged();

                        /* Add node to list */
                        childNodes.append(newChildNodePt);
                        if (!addChildToDB(nodes[j], newChildNodePt, addrType))
                        {
                            cleanImportedVars();
                            exitMemMgmtMode(false);
                            cb(false, "Couldn't Import Database: Please Run Integrity Check");
                            qCritical() << "Couldn't add new child node to DB (corrupted DB?)";
                            return false;
                        }
                    }

                    // Process the next imported child node
                    cur_import_child_node_addr = imported_child_node->getNextChildAddress();
                    cur_import_child_node_addr_v = imported_child_node->getNextChildVirtualAddress();
                }

                // Jump to next service node
                break;
            }
        }

        // Did we find the service core data?
        if(service_node_found == false)
        {
           /* Increment new addresses counter */
           incrementNeededAddresses(MPNode::NodeParent);

           /* Create new node with null address and virtual address set to our counter value */
           MPNode* newNodePt = pMesProt->createMPNode(QByteArray(getParentNodeSize(), 0), this, QByteArray(), newAddressesNeededCounter);
           newNodePt->setType(MPNode::NodeParent);
           newNodePt->setLoginNodeData(importNodes[i]->getNodeFlags(), importNodes[i]->getLoginNodeData());
           newNodePt->setMergeTagged();

           /* Add node to list */
           nodes.append(newNodePt);
           if (!addOrphanParentToDB(newNodePt, false, false, addrType))
           {
               cleanImportedVars();
               exitMemMgmtMode(false);
               qCritical() << "Couldn't add parent to DB (corrupted DB?)";
               cb(false, "Couldn't Import Database: Please Run Integrity Check");
               return false;
           }

           /* Next step is to follow the children */
           QByteArray curImportChildAddr = importNodes[i]->getStartChildAddress();
           while (curImportChildAddr != MPNode::EmptyAddress)
           {
               /* Find node in list */
               MPNode* curImportChildPt = findNodeWithAddressInList(importChildNodes, curImportChildAddr);

               if (!curImportChildPt)
               {
                   cleanImportedVars();
                   exitMemMgmtMode(false);
                   cb(false, "Couldn't Import Database: Corrupted Import File");
                   qCritical() << "Couldn't find import child (import file problem?)";
                   return false;
               }

               /* Increment new addresses counter */
               incrementNeededAddresses(MPNode::NodeChild);

               /* Create new node with null address and virtual address set to our counter value */
               MPNode* newChildNodePt = pMesProt->createMPNode(QByteArray(getChildNodeSize(), 0), this, QByteArray(), newAddressesNeededCounter);
               newChildNodePt->setType(MPNode::NodeChild);
               newChildNodePt->setLoginChildNodeData(curImportChildPt->getNodeFlags(), curImportChildPt->getLoginChildNodeData());
               newChildNodePt->setMergeTagged();

               /* Add node to list */
               childNodes.append(newChildNodePt);
               if (!addChildToDB(newNodePt, newChildNodePt, addrType))
               {
                   cleanImportedVars();
                   exitMemMgmtMode(false);
                   cb(false, "Couldn't Import Database: Please Run Integrity Check");
                   qCritical() << "Couldn't add new child node to DB (corrupted DB?)";
                   return false;
               }

               /* Go to the next child */
               curImportChildAddr = curImportChildPt->getNextChildAddress();
           }
        }
    }
    return true;
}

bool MPDevice::checkImportedDataNodes(const MessageHandlerCb &cb)
{
    /// Find the data nodes we don't have in memory or that have been changed
    for (qint32 i = 0; i < importedDataNodes.size(); i++)
    {
        bool service_node_found = false;
        quint32 encDataSize = 0;

        // Loop in the memory nodes to compare data
        for (qint32 j = 0; j < dataNodes.size(); j++)
        {
            if ((importedDataNodes[i]->getService() == dataNodes[j]->getService()) && (importedDataNodes[i]->getStartDataCtr() == dataNodes[j]->getStartDataCtr()))
            {
                // We found a parent data node that has the same core data (doesn't mean the same prev / next node though!)
                qDebug() << "Data parent node core data match for " << importedDataNodes[i]->getService();
                dataNodes[j]->setMergeTagged();
                service_node_found = true;

                // Next step is to check if the children are the same
                quint32 cur_import_child_node_addr_v = importedDataNodes[i]->getStartChildVirtualAddress();
                QByteArray cur_import_child_node_addr = importedDataNodes[i]->getStartChildAddress();
                quint32 cur_matched_child_node_addr_v = dataNodes[j]->getStartChildVirtualAddress();
                QByteArray cur_matched_child_node_addr = dataNodes[j]->getStartChildAddress();
                MPNode* prev_matched_child_node = nullptr;
                MPNode* matched_child_node = nullptr;
                bool data_match_ongoing = true;

                /* Special case: parent doesn't have children but we do */
                if (((cur_import_child_node_addr == MPNode::EmptyAddress) || (cur_import_child_node_addr.isNull() && cur_import_child_node_addr_v == 0)) && ((cur_matched_child_node_addr != MPNode::EmptyAddress) || (cur_matched_child_node_addr.isNull() && cur_matched_child_node_addr_v != 0)))
                {
                    dataNodes[j]->setStartChildAddress(MPNode::EmptyAddress);
                }

                //qDebug() << "First child address for imported data node: " << cur_import_child_node_addr.toHex() << " , for own node: " << matched_parent_first_child.toHex();
                while ((cur_import_child_node_addr != MPNode::EmptyAddress) || (cur_import_child_node_addr.isNull() && cur_import_child_node_addr_v != 0))
                {
                    // Find the imported child node in our list
                    MPNode* imported_child_node = findNodeWithAddressInList(importedDataChildNodes, cur_import_child_node_addr, cur_import_child_node_addr_v);
                    encDataSize += MP_NODE_DATA_ENC_SIZE;

                    // Check if we actually found the node
                    if (!imported_child_node)
                    {
                        cleanImportedVars();
                        exitMemMgmtMode(false);
                        cb(false, "Couldn't Import Database: Corrupted Import File");
                        qCritical() << "Couldn't find imported data child node in our list (corrupted import file?)";
                        return false;
                    }

                    // If we are still matching, check that we still can
                    if (data_match_ongoing)
                    {
                        if ((cur_matched_child_node_addr == MPNode::EmptyAddress) || (cur_matched_child_node_addr.isNull() && cur_matched_child_node_addr_v == 0))
                        {
                            /* No next node */
                            qDebug() << "Matched imported data child node chain is longer than what we have";
                            data_match_ongoing = false;
                        }
                        else
                        {
                            matched_child_node = findNodeWithAddressInList(dataChildNodes, cur_matched_child_node_addr, cur_matched_child_node_addr_v);

                            // Check if we actually found the node
                            if (!matched_child_node)
                            {
                                cleanImportedVars();
                                exitMemMgmtMode(false);
                                cb(false, "Couldn't Import Database: Please Run Integrity Check");
                                qCritical() << "Couldn't find imported data child node in our list (corrupted DB?)";
                                return false;
                            }

                            // Check for data match
                            if (matched_child_node->getDataChildNodeData() != imported_child_node->getDataChildNodeData())
                            {
                                qDebug() << "Data child node mismatch for " << importedDataNodes[i]->getService();
                                data_match_ongoing = false;

                                /* Chain broken, delete all following data blocks */
                                while ((cur_matched_child_node_addr != MPNode::EmptyAddress) || (cur_matched_child_node_addr.isNull() && cur_matched_child_node_addr_v != 0))
                                {
                                    matched_child_node = findNodeWithAddressInList(dataChildNodes, cur_matched_child_node_addr, cur_matched_child_node_addr_v);

                                    // Check if we actually found the node
                                    if (!matched_child_node)
                                    {
                                        cleanImportedVars();
                                        exitMemMgmtMode(false);
                                        cb(false, "Couldn't Import Database: Please Run Integrity Check");
                                        qCritical() << "Couldn't find imported data child node in our list (corrupted DB?)";
                                        return false;
                                    }

                                    /* Next item */
                                    cur_matched_child_node_addr = matched_child_node->getNextChildDataAddress();
                                    cur_matched_child_node_addr_v = matched_child_node->getNextChildVirtualAddress();

                                    /* Delete current block */
                                    dataChildNodes.removeOne(matched_child_node);
                                }
                            }
                            else
                            {
                                matched_child_node->setMergeTagged();
                            }
                        }
                    }

                    // If we stopped matching the child nodes, add child node data
                    if (!data_match_ongoing)
                    {
                        qDebug() << importedDataNodes[i]->getService() << " : appending child data in the mooltipass...";

                        /* Increment new addresses counter */
                        incrementNeededAddresses(MPNode::NodeChild);

                        /* Create new node with null address and virtual address set to our counter value */
                        MPNode* newDataChildNodePt = pMesProt->createMPNode(QByteArray(getChildNodeSize(), 0), this, QByteArray(), newAddressesNeededCounter);
                        newDataChildNodePt->setType(MPNode::NodeChild);
                        newDataChildNodePt->setDataChildNodeData(imported_child_node->getNodeFlags(), imported_child_node->getDataChildNodeData());
                        newDataChildNodePt->setMergeTagged();

                        /* Add node to list */
                        dataChildNodes.append(newDataChildNodePt);
                        if (!prev_matched_child_node)
                        {
                            /* First node */
                            dataNodes[j]->setStartChildAddress(QByteArray(), newAddressesNeededCounter);
                        }
                        else
                        {
                            prev_matched_child_node->setNextChildDataAddress(QByteArray(), newAddressesNeededCounter);
                        }

                        /* Update prev matched child node */
                        prev_matched_child_node = newDataChildNodePt;
                    }

                    // Fetch next matched child if comparison is still ongoing
                    if (data_match_ongoing)
                    {
                        prev_matched_child_node = matched_child_node;
                        matched_child_node->setMergeTagged();
                        cur_matched_child_node_addr = matched_child_node->getNextChildDataAddress();
                        cur_matched_child_node_addr_v = matched_child_node->getNextChildVirtualAddress();
                    }

                    cur_import_child_node_addr = imported_child_node->getNextChildDataAddress();
                    cur_import_child_node_addr_v = imported_child_node->getNextChildVirtualAddress();
                }

                // Jump to next service node
                break;
            }
        }

        // Did we find the service core data?
        if(service_node_found == false)
        {
           /* Increment new addresses counter */
           incrementNeededAddresses(MPNode::NodeParent);

           /* Create new node with null address and virtual address set to our counter value */
           MPNode* newNodePt = pMesProt->createMPNode(QByteArray(getParentNodeSize(), 0), this, QByteArray(), newAddressesNeededCounter);
           newNodePt->setType(MPNode::NodeParent);
           newNodePt->setLoginNodeData(importedDataNodes[i]->getNodeFlags(), importedDataNodes[i]->getLoginNodeData());
           newNodePt->setMergeTagged();

           /* Add node to list */
           dataNodes.append(newNodePt);
           if (!addOrphanParentToDB(newNodePt, true, false))
           {
               cleanImportedVars();
               exitMemMgmtMode(false);
               qCritical() << "Couldn't add data parent to DB (corrupted DB?)";
               cb(false, "Couldn't Import Database: Please Run Integrity Check");
               return false;
           }

           /* Next step is to follow the children */
           QByteArray curImportChildAddr = importedDataNodes[i]->getStartChildAddress();
           MPNode* prev_added_child_node = nullptr;
           while (curImportChildAddr != MPNode::EmptyAddress)
           {
               /* Find node in list */
               MPNode* curImportChildPt = findNodeWithAddressInList(importedDataChildNodes, curImportChildAddr);
               encDataSize += MP_NODE_DATA_ENC_SIZE;

               if (!curImportChildPt)
               {
                   cleanImportedVars();
                   exitMemMgmtMode(false);
                   cb(false, "Couldn't Import Database: Corrupted Database File");
                   qCritical() << "Couldn't find import child (import file problem?)";
                   return false;
               }

               /* Increment new addresses counter */
               incrementNeededAddresses(MPNode::NodeChild);

               /* Create new node with null address and virtual address set to our counter value */
               MPNode* newDataChildNodePt = pMesProt->createMPNode(QByteArray(getChildNodeSize(), 0), this, QByteArray(), newAddressesNeededCounter);
               newDataChildNodePt->setType(MPNode::NodeChild);
               newDataChildNodePt->setDataChildNodeData(curImportChildPt->getNodeFlags(), curImportChildPt->getDataChildNodeData());
               newDataChildNodePt->setMergeTagged();

               /* Add node to list */
               dataChildNodes.append(newDataChildNodePt);
               if (!prev_added_child_node)
               {
                   /* First node */
                   newNodePt->setStartChildAddress(QByteArray(), newAddressesNeededCounter);
               }
               else
               {
                   prev_added_child_node->setNextChildDataAddress(QByteArray(), newAddressesNeededCounter);
               }

               /* Go to the next child */
               curImportChildAddr = curImportChildPt->getNextChildDataAddress();
               prev_added_child_node = newDataChildNodePt;
           }

           /* Update data size property */
           newNodePt->setEncDataSize(encDataSize);
        }
    }
    return true;
}

bool MPDevice::finishImportFileMerging(QString &stringError, bool noDelete)
{
    qInfo() << "Finishing Import File Merging...";

    if (!noDelete)
    {
        if (!finishImportLoginNodes(stringError, Common::CRED_ADDR_IDX))
        {
            return false;
        }

        if (isBLE())
        {
            if (!finishImportLoginNodes(stringError, Common::WEBAUTHN_ADDR_IDX))
            {
                return false;
            }
        }

        /* Now we check all our parents and childs for non merge tag */
        QListIterator<MPNode*> j(dataNodes);
        while (j.hasNext())
        {
            MPNode* nodeItem = j.next();

            /* No need to check for merge tagged for parent, as it'll automatically be removed if it doesn't have any child */
            QByteArray curChildNodeAddr = nodeItem->getStartChildAddress();
            bool deleteDataNode = false;

            /* Special case: no child */
            if (curChildNodeAddr == MPNode::EmptyAddress)
            {
                /* Remove parent */
                qDebug() << "Empty data parent " << nodeItem->getService() << " detected, deleting it...";
                removeEmptyParentFromDB(nodeItem, true);
            }

            /* Check every children */
            while (curChildNodeAddr != MPNode::EmptyAddress)
            {
                MPNode* curNode = findNodeWithAddressInList(dataChildNodes, curChildNodeAddr);

                /* Safety checks */
                if (!curNode)
                {
                    qCritical() << "Couldn't find child node in list (error in algo?)";
                    stringError = "Moolticute Internal Error: Please Contact The Team (IFM#2)";
                    cleanImportedVars();
                    return false;
                }

                /* Next item */
                curChildNodeAddr = curNode->getNextDataAddress();

                /* Marked for deletion? */
                if (!curNode->getMergeTagged())
                {
                    /* First child? */
                    if (curNode->getAddress() == nodeItem->getStartChildAddress())
                    {
                        deleteDataNode = true;
                    }

                    /* Delete child */
                    dataChildNodes.removeOne(curNode);
                    nodeItem->removeChild(curNode);
                    delete(curNode);
                }
            }

            /* If parent node is marked for deletion */
            if(deleteDataNode)
            {
                nodeItem->setStartChildAddress(MPNode::EmptyAddress);
                removeEmptyParentFromDB(nodeItem, true);
            }
        }
    }

    /* Favorite syncing */
    qInfo() << "Syncing favorites...";
    for (qint32 i = 0; i < importedFavoritesAddrs.size(); i++)
    {
        MPNode* importedCurParentNode = findNodeWithAddressInList(importedLoginNodes, importedFavoritesAddrs[i].mid(0, 2));
        MPNode* importedCurChildNode = findNodeWithAddressInList(importedLoginChildNodes, importedFavoritesAddrs[i].mid(2,2));
        QByteArray localCurParentNodeAddr = favoritesAddrs[i].mid(0, 2);
        QByteArray localCurChildNodeAddr = favoritesAddrs[i].mid(2, 2);

        // Only compare if both addresses are different than 0
        if (!importedCurChildNode || !importedCurParentNode)
        {
            // Check that the same favorite is also empty on our DB
            if (localCurParentNodeAddr != MPNode::EmptyAddress || localCurChildNodeAddr != MPNode::EmptyAddress)
            {
                qDebug() << "Empty or invalid favorite, deleting it on MP...";
                favoritesAddrs[i] = QByteArray(4, 0);
            }
        }
        else
        {
            MPNode* localCurParentNode = findNodeWithNameInList(loginNodes, importedCurParentNode->getService(), true);
            MPNode* localCurChildNode = nullptr;
            if (localCurParentNode)
            {
                localCurChildNode = findNodeWithLoginWithGivenParentInList(loginChildNodes, localCurParentNode, importedCurChildNode->getLogin());
            }

            if (!localCurChildNode || !localCurParentNode)
            {
                qCritical() << "Couldn't find matching favorite " << importedCurParentNode->getService() << " & " << importedCurChildNode->getLogin();
                favoritesAddrs[i] = QByteArray(4, 0);
            }
            else if((localCurParentNode->getAddress() == localCurParentNodeAddr) && (localCurChildNode->getAddress() == localCurChildNodeAddr))
            {
                qDebug() << "Found matching favorite " << importedCurParentNode->getService() << " & " << importedCurChildNode->getLogin();
            }
            else
            {
                qDebug() << "New favorite " << importedCurParentNode->getService() << " & " << importedCurChildNode->getLogin();
                favoritesAddrs[i].replace(0, 2, localCurParentNode->getAddress());
                favoritesAddrs[i].replace(2, 2, localCurChildNode->getAddress());
            }
        }
    }

    /* Check that we didn't do a mess */
    if (!checkLoadedNodes(true, !isMooltiAppImportFile, false))
    {
        qCritical() << "Error in merging algorithm... please contact the devs";
        stringError = "Moolticute Internal Error: Please Contact The Team (IFM#3)";
        cleanImportedVars();
        return false;
    }

    /* Now that we're here, update the change numbers */
    if (!isMooltiAppImportFile)
    {
        set_credentialsDbChangeNumber(importedCredentialsDbChangeNumber);
        set_dataDbChangeNumber(importedDataDbChangeNumber);

        emit dbChangeNumbersChanged(importedCredentialsDbChangeNumber, importedDataDbChangeNumber);
    }
    return true;
}

bool MPDevice::finishImportLoginNodes(QString &stringError, Common::AddressType addrType)
{
    const bool isCred = Common::CRED_ADDR_IDX == addrType;
    NodeList& nodes = isCred ? loginNodes : webAuthnLoginNodes;
    NodeList& childNodes = isCred? loginChildNodes : webAuthnLoginChildNodes;
    /* Now we check all our parents and childs for non merge tag */
    QListIterator<MPNode*> i(nodes);
    while (i.hasNext())
    {
        MPNode* nodeItem = i.next();

        /* No need to check for merge tagged for parent, as it'll automatically be removed if it doesn't have any child */
        QByteArray curChildNodeAddr = nodeItem->getStartChildAddress();

        /* Special case: no child */
        if (curChildNodeAddr == MPNode::EmptyAddress)
        {
            /* Remove parent */
            removeEmptyParentFromDB(nodeItem, false, addrType);
        }

        /* Check every children */
        while (curChildNodeAddr != MPNode::EmptyAddress)
        {
            MPNode* curNode = findNodeWithAddressInList(childNodes, curChildNodeAddr);

            /* Safety checks */
            if (!curNode)
            {
                qCritical() << "Couldn't find child node in list (error in algo?)";
                stringError = "Moolticute Internal Error: Please Contact The Team (IFM#1)";
                cleanImportedVars();
                return false;
            }

            /* Next item */
            curChildNodeAddr = curNode->getNextChildAddress();

            /* Marked for deletion? */
            if (!curNode->getMergeTagged())
            {
                removeChildFromDB(nodeItem, curNode, true, true, addrType);
            }
        }
    }
    return true;
}

void MPDevice::loadFreeAddresses(AsyncJobs *jobs, const QByteArray &addressFrom, bool discardFirstAddr, const MPDeviceProgressCb &cbProgress)
{
    qDebug() << "Loading free addresses from address:" << addressFrom.toHex();

    if (isBLE())
    {
        bleImpl->getFreeAddressProvider().loadFreeAddresses(jobs, addressFrom, cbProgress);
        return;
    }

    jobs->append(new MPCommandJob(this, MPCmd::GET_FREE_ADDRESSES,
                                  addressFrom,
                                  [this, jobs, discardFirstAddr, cbProgress](const QByteArray &data, bool &) -> bool
    {
        quint32 nb_free_addresses_received = pMesProt->getMessageSize(data)/2;
        if (discardFirstAddr)
        {
            nb_free_addresses_received--;
        }

        qDebug() << "Received " << nb_free_addresses_received << " free addresses";
        QVariantMap progressData = { {"total", newAddressesNeededCounter},
                                     {"current", newAddressesReceivedCounter + nb_free_addresses_received},
                                     {"msg", "%1 Free Addresses Received"},
                                     {"msg_args", QVariantList({newAddressesReceivedCounter})}
                                   };
        cbProgress(progressData);

        if (nb_free_addresses_received == 0)
        {
            /* No more free addresses */
            jobs->setCurrentJobError("No more free addresses to get");
            qCritical() << "No more free addresses to get";
            return false;
        }
        else
        {
            /* Add the free addresses to our buffer */
            for (quint32 i = 0; i < nb_free_addresses_received; i++)
            {
                if (discardFirstAddr)
                {
                    freeAddresses.append(pMesProt->getPayloadBytes(data, 2 + i*2, 2));

                    if (AppDaemon::isDebugDev())
                        qDebug() << "Received free address " << pMesProt->getPayloadBytes(data, 2 + i*2, 2).toHex();
                }
                else
                {
                    freeAddresses.append(pMesProt->getPayloadBytes(data,i*2, 2));

                    if (AppDaemon::isDebugDev())
                        qDebug() << "Received free address " << pMesProt->getPayloadBytes(data,i*2, 2).toHex();
                }
            }

            /* Increment counter */
            newAddressesReceivedCounter += nb_free_addresses_received;

            /* Did we receive enough addresses ? */
            if (newAddressesReceivedCounter > newAddressesNeededCounter)
            {
                qDebug() << "Received enough free addresses";
            }
            else
            {
                /* Ask more addresses */
                qDebug() << "Still needing " << newAddressesNeededCounter - newAddressesReceivedCounter << " free addresses";
                loadFreeAddresses(jobs, freeAddresses[freeAddresses.size()-1], true, cbProgress);
            }

            return true;
        }
    }));
}

void MPDevice::incrementNeededAddresses(MPNode::NodeType type)
{
    ++newAddressesNeededCounter;
    if (isBLE())
    {
        if (MPNode::NodeParent == type)
        {
            bleImpl->getFreeAddressProvider().incrementParentNodeNeeded(newAddressesNeededCounter);
        }
        else if (MPNode::NodeChild == type)
        {
            bleImpl->getFreeAddressProvider().incrementChildNodeNeeded(newAddressesNeededCounter);
        }
        else
        {
            qCritical() << "Invalid MPNode type: " << type;
        }
    }
}

void MPDevice::startIntegrityCheck(const std::function<void(bool success, int freeBlocks, int totalBlocks, QString errstr)> &cb,
                                   const MPDeviceProgressCb &cbProgress)
{
    /* New job for starting MMM */
    AsyncJobs *jobs = new AsyncJobs("Starting integrity check", this);

    /* Ask device to go into MMM first */
    jobs->append(new MPCommandJob(this, MPCmd::START_MEMORYMGMT, pMesProt->getDefaultFuncDone()));

    /* Ask one free address just in case we need it for creating a _recovered_ service */
    newAddressesNeededCounter = 1;
    newAddressesReceivedCounter = 0;
    loadFreeAddresses(jobs, MPNode::EmptyAddress, false, cbProgress);

    /* Setup global vars dedicated to speed diagnostics */
    diagNbBytesRec = 0;
    diagLastNbBytesPSec = 0;
    lastFlashPageScanned = 0;
    diagLastSecs = QDateTime::currentMSecsSinceEpoch()/1000;
    diagFreeBlocks = 0;
    diagTotalBlocks = 0;

    /* Load CTR, favorites, nodes... */
    memMgmtModeReadFlash(jobs, true, cbProgress, true, true, true);

    connect(jobs, &AsyncJobs::finished, [this, cb](const QByteArray &)
    {
        qInfo() << "Finished loading the nodes in memory";
        qInfo() << "Total blocks:" << diagTotalBlocks;
        qInfo() << "Free blocks:" << diagFreeBlocks;
        qInfo() << "Available blocks:" << diagTotalBlocks - diagFreeBlocks;
        qInfo() << "Available credentials:" << (diagTotalBlocks - diagFreeBlocks) / 2;

        /* We finished loading the nodes in memory */
        AsyncJobs* repairJobs = new AsyncJobs("Checking memory contents...", this);

        /* Let's corrupt the DB for fun */
        //testCodeAgainstCleanDBChanges(repairJobs);

        /* Check loaded nodes, set bool to repair */
        checkLoadedNodes(true, true, true);

        /* Just in case a new _recovered_ service was added, change virtual for real addresses */
        changeVirtualAddressesToFreeAddresses();

        /* set clone change number to actual, to prevent change number changes on device */
        credentialsDbChangeNumberClone = get_credentialsDbChangeNumber();
        dataDbChangeNumberClone = get_dataDbChangeNumber();

        /* Generate save packets */
        bool packets_generated = generateSavePackets(repairJobs, true, true, [](QVariantMap){});

        /* Leave MMM */
        repairJobs->append(new MPCommandJob(this, MPCmd::END_MEMORYMGMT, pMesProt->getDefaultFuncDone()));

        connect(repairJobs, &AsyncJobs::finished, [this, packets_generated, cb](const QByteArray &data)
        {
            Q_UNUSED(data);

            if (packets_generated)
            {
                qInfo() << "Found and Corrected Errors in Database";
                cb(true, diagFreeBlocks, diagTotalBlocks, "Errors Were Found And Corrected In The Database");
            }
            else
            {
                qInfo() << "Nothing to correct in DB";
                cb(true, diagFreeBlocks, diagTotalBlocks, "Database Is Free Of Errors");
            }
        });

        connect(repairJobs, &AsyncJobs::failed, [this, cb](AsyncJob *failedJob)
        {
            Q_UNUSED(failedJob);
            qCritical() << "Couldn't check memory contents";
            cb(false, diagFreeBlocks, diagTotalBlocks, "Error While Correcting Database (Device Disconnected?)");
        });

        jobsQueue.enqueue(repairJobs);
        runAndDequeueJobs();
    });

    connect(jobs, &AsyncJobs::failed, [this, cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Failed scanning the flash memory";
        cb(false, diagFreeBlocks, diagTotalBlocks, "Couldn't scan the complete memory (Device Disconnected?)");
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::serviceExists(bool isDatanode, QString service, const QString &reqid,
                             std::function<void(bool success, QString errstr, QString service, bool exists)> cb)
{
    if (service.isEmpty())
    {
        qWarning() << "context is empty.";
        cb(false, "context is empty", QString(), false);
        return;
    }

    //Force all service names to lowercase
    service = service.toLower();

    QString logInf = QStringLiteral("Check if %1service exists: %2 reqid: %3")
                     .arg(isDatanode?"data ":"credential ")
                     .arg(service)
                     .arg(reqid);

    AsyncJobs *jobs;
    if (reqid.isEmpty())
        jobs = new AsyncJobs(logInf, this);
    else
        jobs = new AsyncJobs(logInf, reqid, this);

    QByteArray sdata = pMesProt->toByteArray(service);
    sdata.append((char)0);

    if (isBLE())
    {
        sdata.append((char)0);
        jobs->append(new MPCommandJob(this, MPCmd::CHECK_DATA_FILE,
                                      sdata,
                  [this, jobs, service](const QByteArray &data, bool &)
                    {
                        QVariantMap m = {{ "service", service },
                                         { "exists", pMesProt->getFirstPayloadByte(data) == MSG_SUCCESS }};
                        jobs->user_data = m;
                        return true;
                    }
                  ));
    }
    else
    {
        jobs->append(new MPCommandJob(this, isDatanode? MPCmd::SET_DATA_SERVICE : MPCmd::CONTEXT,
                                      sdata,
                                      [this, jobs, service](const QByteArray &data, bool &) -> bool
        {
            QVariantMap m = {{ "service", service },
                             { "exists", pMesProt->getFirstPayloadByte(data) == 1 }};
            jobs->user_data = m;
            return true;
        }));
    }

    connect(jobs, &AsyncJobs::finished, [jobs, cb](const QByteArray &)
    {
        //all jobs finished success
        qInfo() << "service_exists success";
        QVariantMap m = jobs->user_data.toMap();
        cb(true, QString(), m["service"].toString(), m["exists"].toBool());
    });

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        qCritical() << "Failed getting data node";
        cb(false, failedJob->getErrorStr(), QString(), false);
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}


void MPDevice::importFromCSV(const QJsonArray &creds, const MPDeviceProgressCb &cbProgress,
                   MessageHandlerCb cb)
{
    /* Loop through credentials to check them */
    for (qint32 i = 0; i < creds.size(); i++)
    {
        /* Create object */
        QJsonObject qjobject = creds[i].toObject();

        /* Check login size */
        if (qjobject["login"].toString().length() >= pMesProt->getLoginMaxLength()-1)
        {
            cb(false, "Couldn't import CSV file: " + qjobject["login"].toString() + " has longer than supported length");
            return;
        }

        /* Check password size */
        if (qjobject["password"].toString().length() >= pMesProt->getPwdMaxLength()-1)
        {
            cb(false, "Couldn't import CSV file: " + qjobject["password"].toString() + " has longer than supported length");
            return;
        }
    }

    /* Load database credentials */
    AsyncJobs *jobs = new AsyncJobs("Starting MMM mode for CSV import", this);

    /* Ask device to go into MMM first */
    auto startMmmJob = new MPCommandJob(this, MPCmd::START_MEMORYMGMT, pMesProt->getDefaultFuncDone());
    startMmmJob->setTimeout(15000); //We need a big timeout here in case user enter a wrong pin code
    jobs->append(startMmmJob);

    /* Load flash contents the usual way */
    memMgmtModeReadFlash(jobs, false, cbProgress, true, false, true);

    connect(jobs, &AsyncJobs::finished, [this, creds, cb, cbProgress](const QByteArray &data)
    {
        Q_UNUSED(data)

        /* Tag favorites */
        tagFavoriteNodes();

        /* Check DB */
        if (checkLoadedNodes(true, false, false))
        {
            qInfo() << "Mem management mode enabled, DB checked";

            /* Array containing our processed credentials */
            QJsonArray creds_processed;

            /* In case of duplicate credentials, fill the node addresses */
            for (qint32 i = 0; i < creds.size(); i++)
            {
                /* Create object */
                QJsonObject qjobject = creds[i].toObject();

                /* Parse URL */
                QString importedURL = qjobject["service"].toString();
                ParseDomain url(importedURL);

                /* Format imported URL */
                if (url.isWebsite())
                {
                    if (!url.subdomain().isEmpty())
                    {
                        qjobject["service"] = url.getFullSubdomain();
                    }
                    else
                    {
                        qjobject["service"] = url.getFullDomain();
                    }
                }

                /* Debug */
                qDebug() << importedURL << "converted to:" << qjobject["service"].toString();

                /* To reuse setMMCredentials() we add the required fields */
                qjobject["description"] = "imported from CSV";
                qjobject["favorite"] = -1;

                if (isBLE())
                {
                    qjobject["category"] = 0;
                    const int DEFAULT_KEY_AFTER = 0xFFFF;
                    qjobject["key_after_login"] = DEFAULT_KEY_AFTER;
                    qjobject["key_after_pwd"] = DEFAULT_KEY_AFTER;
                }

                /* Try to find same service */
                MPNode* parentPt = findNodeWithServiceInList(qjobject["service"].toString());

                if(!parentPt)
                {
                    /* New service, leave empty address */
                    qjobject["address"] = QJsonArray();
                    qInfo() << "CSV import: new login" << qjobject["login"].toString() << "for new service" << qjobject["service"].toString();
                }
                else
                {
                    /* Service found, try to find login that has the same name */
                    MPNode* childPt = findNodeWithLoginWithGivenParentInList(loginChildNodes, parentPt, qjobject["login"].toString());

                    if(!childPt)
                    {
                        /* New service, leave empty address */
                        qjobject["address"] = QJsonArray();
                        qInfo() << "CSV import: new login" << qjobject["login"].toString() << "for existing service" << qjobject["service"].toString();
                    }
                    else
                    {
                        /* Update address with existing one */
                        qjobject["address"] = QJsonArray({{ childPt->getAddress().at(0) }, { childPt->getAddress().at(1) }});
                        qInfo() << "CSV import: updated password for login" << qjobject["login"].toString() << "for existing service" << qjobject["service"].toString();
                    }
                }

                /* Add credential to list */
                creds_processed.append(qjobject);
            }

            /* finally, call setmmccredentials */
            setMMCredentials(creds_processed, true, cbProgress, cb, true);
        }
        else
        {
            qInfo() << "DB has errors, leaving MMM";
            exitMemMgmtMode(false);
            cb(false, "Database Contains Errors, Please Run Integrity Check");
        }
    });

    connect(jobs, &AsyncJobs::failed, [this, cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob)
        qCritical() << "Setting device in MMM failed";
        exitMemMgmtMode(false);
        cb(false, "Couldn't Load Database, Please Approve Prompt On Device");
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::setMMCredentials(const QJsonArray &creds, bool noDelete,
                                const MPDeviceProgressCb &cbProgress,
                                MessageHandlerCb cb, bool isCsv /* = false */)
{
    newAddressesNeededCounter = 0;
    newAddressesReceivedCounter = 0;
    bool packet_send_needed = false;
    bool onlyChangePwd = isBLE() && isCsv && !bleImpl->get_advancedMenu();
    AsyncJobs *jobs = new AsyncJobs("Merging credentials changes", this);

    /// TODO: sanitize inputs (or not, as it is done at the mpnode.cpp level)

    /* Look for deleted or changed nodes */
    for (qint32 i = 0; i < creds.size(); i++)
    {
        /* Create object */
        QJsonObject qjobject = creds[i].toObject();

        /* Check format */
        bool isTotpCred = isBLE() && (MessageProtocolBLE::TOTP_PACKAGE_SIZE == qjobject.size());
        if (qjobject.size() != pMesProt->getCredentialPackageSize() && !isTotpCred)
        {
            qCritical() << "Unknown JSON return format:" << qjobject;
            cb(false, "Wrong JSON formated credential list");
            return;
        }
        else
        {
            /* Credential data */
            QByteArray nodeAddr;
            QString login = qjobject["login"].toString();
            qint32 favorite = qjobject["favorite"].toInt();
            QString service = qjobject["service"].toString().toLower();
            QString password = qjobject["password"].toString();
            QString description = qjobject["description"].toString();
            QJsonArray addrArray = qjobject["address"].toArray();
            int category = 0;
            int keyAfterLogin = 0;
            int keyAfterPwd = 0;
            if (isBLE())
            {
                category = qjobject["category"].toInt();
                keyAfterLogin = qjobject["key_after_login"].toInt();
                keyAfterPwd = qjobject["key_after_pwd"].toInt();
                if (isTotpCred)
                {
                    bleImpl->createTOTPCredMessage(service, login, qjobject["totp"].toObject());
                }
            }
            for (qint32 j = 0; j < addrArray.size(); j++) { nodeAddr.append(addrArray[j].toInt()); }
            qDebug() << "MMM Save: tackling " << login << " for service " << service << " at address " << nodeAddr.toHex();

            /* Find node in our list */
            MPNode* nodePtr = findNodeWithAddressInList(loginChildNodes, nodeAddr);

            /* If not a new node, look for parent Node */
            MPNode* parentNodePtr = nullptr;
            bool loginHasNewParent = false;
            if (!nodeAddr.isNull())
            {
                /* Find parent node that has as a children the currently investigated one */
                parentNodePtr = findCredParentNodeGivenChildNodeAddr(nodeAddr, 0);
                if (!parentNodePtr)
                {
                    qCritical() << "Error in our local DB (algo PB?)";
                    cb(false, "Moolticute Internal Error (SMMC#4)");
                    exitMemMgmtMode(true);
                    return;
                }

                /* Check its name */
                if (service != parentNodePtr->getService())
                {
                    qDebug() << "Login" << login << " has new service " << service;
                    loginHasNewParent = true;
                }
            }

            if (nodeAddr.isNull())
            {
                qDebug() << "New login" << qjobject["login"].toString() << " for service " << qjobject["service"].toString() << " at address " << nodeAddr.toHex();

                /* Look if there's a parent node with that name */
                MPNode* parentPtr = findNodeWithServiceInList(service);

                /* If no parent, create it */
                if (!parentPtr)
                {
                    parentPtr = addNewServiceToDB(service);
                }

                /* Increment new addresses counter */
                incrementNeededAddresses(MPNode::NodeChild);

                /* Create new node with null address and virtual address set to our counter value */
                MPNode* newNodePt = pMesProt->createMPNode(QByteArray(getChildNodeSize(), 0), this, QByteArray(), newAddressesNeededCounter);
                newNodePt->setType(MPNode::NodeChild);
                loginChildNodes.append(newNodePt);
                newNodePt->setNotDeletedTagged();
                newNodePt->setLogin(login);
                newNodePt->setDescription(description);
                if (isBLE())
                {
                    bleImpl->setNodeCategory(newNodePt, category);
                    bleImpl->setNodeKeyAfterLogin(newNodePt, keyAfterLogin);
                    bleImpl->setNodeKeyAfterPwd(newNodePt, keyAfterPwd);
                    bleImpl->setNodePwdBlankFlag(newNodePt);
                }
                addChildToDB(parentPtr, newNodePt);
                packet_send_needed = true;

                /* Set favorite */
                newNodePt->setFavoriteProperty(favorite);

                /* Finally, change password */
                if (onlyChangePwd)
                {
                    mmmPasswordChangeNewAddrArray.insert(newNodePt->getVirtualAddress(), ChangeElem{password});
                }
                else
                {
                    QStringList changeList;
                    changeList << service << login << password;
                    mmmPasswordChangeArray.append(changeList);
                }
                qDebug() << "Queing password change as well";
            }
            else if (!nodePtr)
            {
                qCritical() << "Couldn't find" << qjobject["login"].toString() << " for service " << qjobject["service"].toString() << " at address " << nodeAddr.toHex();
                cb(false, "Moolticute Internal Error (SMMC#1)");
                exitMemMgmtMode(true);
                return;
            }
            else if (loginHasNewParent)
            {
                /* Look if there's a parent node with that name */
                MPNode* parentPtr = findNodeWithServiceInList(service);

                /* If no parent, create it */
                if (!parentPtr)
                {
                    parentPtr = addNewServiceToDB(service);
                }

                /* Increment new addresses counter */
                incrementNeededAddresses(MPNode::NodeParent);

                /* Remove child from previous parent */
                removeChildFromDB(parentNodePtr, nodePtr, true, false);

                /* Overwrite login & description just in case they changed */
                nodePtr->setNotDeletedTagged();
                nodePtr->setLogin(login);
                nodePtr->setDescription(description);
                nodePtr->setFavoriteProperty(favorite);
                if (isBLE())
                {
                    bleImpl->setNodeCategory(nodePtr, category);
                    bleImpl->setNodeKeyAfterLogin(nodePtr, keyAfterLogin);
                    bleImpl->setNodeKeyAfterPwd(nodePtr, keyAfterPwd);
                }
                addChildToDB(parentPtr, nodePtr);

                /* Check for changed password */
                if (!password.isEmpty())
                {
                    qDebug() << "Detected password change for" << login << "on" << service;

                    if (onlyChangePwd)
                    {
                        mmmPasswordChangeExistingAddrArray.insert(nodePtr->getAddress(), password);
                    }
                    {
                        QStringList changeList;
                        changeList << service << login << password;
                        mmmPasswordChangeArray.append(changeList);
                    }
                }

                /* Set bool */
                packet_send_needed = true;
            }
            else
            {
                /* Tag it as not deleted */
                nodePtr->setNotDeletedTagged();

                /* Check if favorite is different */
                if (favorite != nodePtr->getFavoriteProperty())
                {
                    qDebug() << "Favorite id change for login" << qjobject["login"].toString() << " for service " << qjobject["service"].toString() << " at address " << nodeAddr.toHex();
                    packet_send_needed = true;
                }

                /* Set favorite */
                nodePtr->setFavoriteProperty(favorite);

                /* Check for changed description */
                if (description != nodePtr->getDescription())
                {
                    qDebug() << "Detected description change";
                    nodePtr->setDescription(description);
                    packet_send_needed = true;
                }

                if (isBLE())
                {
                    auto* nodeBle = dynamic_cast<MPNodeBLE*>(nodePtr);
                    if (category != nodeBle->getCategory())
                    {
                        nodeBle->setCategory(category);
                        packet_send_needed = true;
                    }
                    if (keyAfterLogin != nodeBle->getKeyAfterLogin())
                    {
                        nodeBle->setKeyAfterLogin(keyAfterLogin);
                        packet_send_needed = true;
                    }
                    if (keyAfterPwd != nodeBle->getKeyAfterPwd())
                    {
                        nodeBle->setKeyAfterPwd(keyAfterPwd);
                        packet_send_needed = true;
                    }
                }

                /* Check for changed login */
                if (login != nodePtr->getLogin())
                {
                    qDebug() << "Detected login change";

                    /* Look for parent Node */
                    MPNode* parentNodePtr = findCredParentNodeGivenChildNodeAddr(nodePtr->getAddress(), nodePtr->getVirtualAddress());
                    if (!parentNodePtr)
                    {
                        qCritical() << "Couldn't find parent node" << qjobject["service"].toString() << " for login " << qjobject["login"].toString() << " at address " << nodeAddr.toHex();
                        cb(false, "Moolticute Internal Error (SMMC#3)");
                        exitMemMgmtMode(true);
                        return;
                    }

                    /* Create new node, remove the old one and add it */
                    MPNode* newNode = pMesProt->createMPNode(nodePtr->getNodeData(), this, nodePtr->getAddress(), nodePtr->getVirtualAddress());
                    newNode->setLogin(login);
                    newNode->setNotDeletedTagged();
                    loginChildNodes.append(newNode);
                    removeChildFromDB(parentNodePtr, nodePtr, false, true);
                    addChildToDB(parentNodePtr, newNode);
                    packet_send_needed = true;

                    /* Set favorite */
                    newNode->setFavoriteProperty(favorite);
                }

                /* Check for changed password */
                if (!password.isEmpty())
                {
                    qDebug() << "Detected password change for" << login << "on" << service;

                    if (onlyChangePwd)
                    {
                        mmmPasswordChangeExistingAddrArray.insert(nodePtr->getAddress(), password);
                    }
                    else
                    {
                        QStringList changeList;
                        changeList << service << login << password;
                        mmmPasswordChangeArray.append(changeList);
                    }
                    packet_send_needed = true;
                }
            }
        }
    }

    /* Browse through the memory contents to find not nonDeleted nodes */
    QListIterator<MPNode*> i(loginNodes);
    while (i.hasNext())
    {
        MPNode* nodeItem = i.next();

        /* No need to check for notdeleted tagged for parent, as it'll automatically be removed if it doesn't have any child */
        QByteArray curChildNodeAddr = nodeItem->getStartChildAddress();
        quint32 curChildNodeAddr_v = nodeItem->getStartChildVirtualAddress();

        /* Special case: no child */
        if ((curChildNodeAddr == MPNode::EmptyAddress) || (curChildNodeAddr.isNull() && curChildNodeAddr_v == 0))
        {
            /* Remove parent */
            removeEmptyParentFromDB(nodeItem, false);
            packet_send_needed = true;
        }

        /* Check every children */
        while ((curChildNodeAddr != MPNode::EmptyAddress) || (curChildNodeAddr.isNull() && curChildNodeAddr_v != 0))
        {
            MPNode* curNode = findNodeWithAddressInList(loginChildNodes, curChildNodeAddr, curChildNodeAddr_v);

            /* Safety checks */
            if (!curNode)
            {
                qCritical() << "Couldn't find child node in list (corrupted DB?)";
                cb(false, "Database error: please run integrity check");
                exitMemMgmtMode(true);
                return;
            }

            /* Next item */
            curChildNodeAddr = curNode->getNextChildAddress();
            curChildNodeAddr_v = curNode->getNextChildVirtualAddress();

            /* Marked for deletion? */
            if (!noDelete && !curNode->getNotDeletedTagged())
            {
                removeChildFromDB(nodeItem, curNode, true, true);
                packet_send_needed = true;
            }
        }
    }

    /* Double check our work */
    if (!checkLoadedNodes(true, false, false))
    {
        qCritical() << "Error in our local DB (algo PB?)";
        cb(false, "Moolticute Internal Error (SMMC#2)");
        exitMemMgmtMode(true);
        return;
    }

    /* Generate save passwords */
    if (!packet_send_needed)
    {
        if (isBLE() && bleImpl->storeTOTPCreds())
        {
            runAndDequeueJobs();
        }
        qInfo() << "No changes detected";
        cb(true, "No Changes Required");
        exitMemMgmtMode(true);
        return;
    }

    /* Increment db change numbers */
    if (isFw12() || isBLE())
    {
        set_credentialsDbChangeNumber(get_credentialsDbChangeNumber() + 1);
        credentialsDbChangeNumberClone = get_credentialsDbChangeNumber();
        updateChangeNumbers(jobs, Common::CredentialNumberChanged);
    }

    emit dbChangeNumbersChanged(get_credentialsDbChangeNumber(), get_dataDbChangeNumber());

    /* Out of pure coding laziness, ask free addresses even if we don't need them */
    loadFreeAddresses(jobs, MPNode::EmptyAddress, false, cbProgress);

    connect(jobs, &AsyncJobs::finished, [this, cb, cbProgress, isCsv, onlyChangePwd](const QByteArray &)
    {
        qInfo() << "Received enough free addresses";

        /* We got all the addresses, change virtual addrs for real addrs and finish merging */
        if (newAddressesNeededCounter > 0)
        {
            changeVirtualAddressesToFreeAddresses(onlyChangePwd);
        }

        /* Browse through the memory contents to find to store favorites */
        for (qint32 i = 0; i < favoritesAddrs.size(); i++)
        {
            favoritesAddrs[i] = QByteArray(4, 0);
        }
        QListIterator<MPNode*> i(loginChildNodes);
        while (i.hasNext())
        {
            MPNode* nodeItem = i.next();

            if (nodeItem->getFavoriteProperty() >= 0)
            {
                MPNode* parentItem = findCredParentNodeGivenChildNodeAddr(nodeItem->getAddress(), 0);
                QByteArray favAddr = QByteArray();
                favAddr.append(parentItem->getAddress());
                favAddr.append(nodeItem->getAddress());
                favoritesAddrs[nodeItem->getFavoriteProperty()] = favAddr;
            }
        }

        AsyncJobs* mergeOperations = new AsyncJobs("Starting merge operations...", this);
        connect(mergeOperations, &AsyncJobs::finished, [this, cb, cbProgress, isCsv, onlyChangePwd](const QByteArray &data)
        {
            Q_UNUSED(data);

            if (isBLE())
            {
                bleImpl->storeTOTPCreds();
            }

            if (mmmPasswordChangeArray.isEmpty() &&
                    mmmPasswordChangeNewAddrArray.isEmpty() &&
                    mmmPasswordChangeExistingAddrArray.isEmpty())
            {
                cb(true, "Changes Applied to Memory");
                qInfo() << "No passwords to be changed";
                exitMemMgmtMode(true);
                return;
            }

            AsyncJobs *pwdChangeJobs = new AsyncJobs("Changing passwords...", this);

            if (isBLE())
            {
                for (qint32 i = 0; i < mmmPasswordChangeArray.size(); i++)
                {
                    if (isCsv)
                    {
                        if (bleImpl->get_advancedMenu())
                        {
                            bleImpl->storeCredential(BleCredential{mmmPasswordChangeArray[i][0], mmmPasswordChangeArray[i][1], "", "", mmmPasswordChangeArray[i][2]}, cb);
                        }
                        else
                        {
                            break;
                        }
                    }
                    else
                    {
                        bleImpl->checkAndStoreCredential(BleCredential{mmmPasswordChangeArray[i][0], mmmPasswordChangeArray[i][1], "", "", mmmPasswordChangeArray[i][2]}, cb);
                    }
                }
                if (onlyChangePwd)
                {
                    qDebug() << "Changing pwd for new nodes";
                    for (auto& change : mmmPasswordChangeNewAddrArray)
                    {
                        bleImpl->changePassword(change.addr, change.pwd, cb);
                    }

                    qDebug() << "Changing pwd for existing nodes";
                    for (auto& change : mmmPasswordChangeExistingAddrArray.toStdMap())
                    {
                        bleImpl->changePassword(change.first, change.second, cb);
                    }
                }
            }

            exitMemMgmtMode(true);
            qInfo() << "Merge operations succeeded!";

            if (!isBLE())
            {
                /* Create password change jobs */
                for (qint32 i = 0; i < mmmPasswordChangeArray.size(); i++)
                {
                    QByteArray sdata = pMesProt->toByteArray(mmmPasswordChangeArray[i][0]);
                    sdata.append((char)0);

                    //First query if context exist
                    pwdChangeJobs->append(new MPCommandJob(this, MPCmd::CONTEXT,
                                                           sdata,
                                                           [this, i, cbProgress](const QByteArray &data, bool &) -> bool
                    {
                        QVariantMap qmapdata = {
                            {"total", mmmPasswordChangeArray.size()},
                            {"current", i},
                            {"msg", "%1/%2: Please Approve Password Storage" },
                            {"msg_args", QVariantList({mmmPasswordChangeArray[i][0], mmmPasswordChangeArray[i][1]})}
                        };
                        cbProgress(qmapdata);

                        if (pMesProt->getFirstPayloadByte(data) != 1)
                        {
                            qWarning() << "context " << mmmPasswordChangeArray[i][0] << " does not exist";
                            return false;
                        }
                        else
                        {
                            qDebug() << "set_context " << mmmPasswordChangeArray[i][0];
                            return true;
                        }
                    }));

                    QByteArray ldata = pMesProt->toByteArray(mmmPasswordChangeArray[i][1]);
                    ldata.append((char)0);

                    pwdChangeJobs->append(new MPCommandJob(this, MPCmd::SET_LOGIN,
                                                  ldata,
                                                  [this, i](const QByteArray &data, bool &) -> bool
                    {
                        if (pMesProt->getFirstPayloadByte(data) == 0)
                        {
                            this->currentJobs->setCurrentJobError("set_login failed on device");
                            qWarning() << "failed to set login to " << mmmPasswordChangeArray[i][1];
                            return false;
                        }
                        else
                        {
                            qDebug() << "set_login " << mmmPasswordChangeArray[i][1];
                            return true;
                        }
                    }));

                    QByteArray pdata = pMesProt->toByteArray(mmmPasswordChangeArray[i][2]);
                    pdata.append((char)0);

                    pwdChangeJobs->append(new MPCommandJob(this, MPCmd::SET_PASSWORD,
                                                   pdata,
                                                   [this, i](const QByteArray &data, bool &) -> bool
                    {
                        if (pMesProt->getFirstPayloadByte(data) == 0)
                        {
                            this->currentJobs->setCurrentJobError("set_password failed on device");
                            qWarning() << "failed to set_password for " << mmmPasswordChangeArray[i][0];
                            /* Below: no call back as the user can approve the next changes */
                            //return false;
                        }
                        qDebug() << "set_password ok";
                        return true;
                    }));
                }
            }

            connect(pwdChangeJobs, &AsyncJobs::finished, [this, cb](const QByteArray &)
            {
                cb(true, "Changes Applied to Memory");
                qInfo() << "Passwords changed!";
                mmmPasswordChangeArray.clear();
                mmmPasswordChangeNewAddrArray.clear();
                mmmPasswordChangeExistingAddrArray.clear();
            });

            connect(pwdChangeJobs, &AsyncJobs::failed, [this, cb](AsyncJob *failedJob)
            {
                Q_UNUSED(failedJob);
                mmmPasswordChangeArray.clear();
                mmmPasswordChangeNewAddrArray.clear();
                mmmPasswordChangeExistingAddrArray.clear();
                qCritical() << "Couldn't change passwords";
                cb(false, "Please Approve Password Changes On The Device");
            });

            jobsQueue.enqueue(pwdChangeJobs);
            runAndDequeueJobs();
        });
        connect(mergeOperations, &AsyncJobs::failed, [this, cb](AsyncJob *failedJob)
        {
            Q_UNUSED(failedJob);
            exitMemMgmtMode(true);
            qCritical() << "Merge operations failed!";
            cb(false, "Couldn't Apply Modifications: Device Unplugged?");
            return;
        });

        generateSavePackets(mergeOperations, true, false, cbProgress);
        jobsQueue.enqueue(mergeOperations);
        runAndDequeueJobs();
    });

    connect(jobs, &AsyncJobs::failed, [this, cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "MMM save failed: couldn't load enough free addresses";
        cb(false, "Couldn't Save Changes (Device Disconnected?)");
        exitMemMgmtMode(true);
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::exportDatabase(const QString &encryption, std::function<void(bool success, QString errstr, QByteArray fileData)> cb,
                              const MPDeviceProgressCb &cbProgress)
{
    /* New job for starting MMM */
    AsyncJobs *jobs = new AsyncJobs("Starting MMM mode for export file generation", this);

    /* Ask device to go into MMM first */
    jobs->append(new MPCommandJob(this, MPCmd::START_MEMORYMGMT, pMesProt->getDefaultFuncDone()));

    /* Load flash contents the usual way */
    memMgmtModeReadFlash(jobs, false,
                            cbProgress
                            , true, true, true);

    connect(jobs, &AsyncJobs::finished, [this, cb, encryption](const QByteArray &)
    {
        qInfo() << "Memory management mode entered";
        exitMemMgmtMode(false);

        /* Check DB just in case.... */
        if (!checkLoadedNodes(true, true, false))
        {
            qCritical() << "Corrupted DB";
            cb(false, "Couldn't create export file, please run integrity check", QByteArray());
        }
        else
        {
            /* Generate export file */
            cb(true, "Export File Generated!", generateExportFileData(encryption));
        }
    });

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Setting device in MMM failed";
        cb(false, "Please Retry and Approve Credential Management", QByteArray());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::importDatabase(const QByteArray &fileData, bool noDelete,
                              MessageHandlerCb cb,
                              const MPDeviceProgressCb &cbProgress)
{
    QString errorString;

    /* Reset temp vars */
    newAddressesNeededCounter = 0;
    newAddressesReceivedCounter = 0;

    /* Try to read the export file */
    if (readExportFile(fileData, errorString))
    {
        /// We are here because the card is known by the export file and the export file is valid

        /* If we don't know this card, we need to add the CPZ CTR */
        if (get_status() == Common::UnknownSmartcard && !unknownCardAddPayload.isEmpty())
        {
            AsyncJobs* addcpzjobs = new AsyncJobs("Adding CPZ/CTR...", this);

            /* Query change number */
            addcpzjobs->append(new MPCommandJob(this,
                                          MPCmd::ADD_UNKNOWN_CARD,
                                          unknownCardAddPayload,
                                          [this](const QByteArray &data, bool &) -> bool
            {
                return (pMesProt->getFirstPayloadByte(data) != 0);
            }));

            connect(addcpzjobs, &AsyncJobs::finished, [this, cbProgress, cb, noDelete](const QByteArray &data)
            {
                Q_UNUSED(data);
                qInfo() << "CPZ/CTR Added";

                /* Unknown card added, start merging */
                startImportFileMerging(cbProgress, cb, noDelete);
            });

            connect(addcpzjobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
            {
                Q_UNUSED(failedJob);
                qCritical() << "Adding unknown card failed";
                cb(false, "Please Retry and Enter Your Card PIN");
            });

            jobsQueue.enqueue(addcpzjobs);
            runAndDequeueJobs();
        }
        else
        {
            /* Start merging */
            startImportFileMerging(cbProgress, cb, noDelete);
        }
    }
    else
    {
        /* Something went wrong during export file reading */
        cb(false, errorString);
    }
}

QList<QVariantMap> MPDevice::getFilesCache()
{
    return filesCache.load();
}

bool MPDevice::hasFilesCache()
{
    return filesCache.exist();
}

bool MPDevice::isFilesCacheInSync() const
{
    return filesCache.isInSync();
}

void MPDevice::getStoredFiles(std::function<void (bool, QList<QVariantMap>)> cb)
{
    /* New job for starting MMM */
    AsyncJobs *jobs = new AsyncJobs("Starting MMM mode", this);

    /* Ask device to go into MMM first */
    jobs->append(new MPCommandJob(this, MPCmd::START_MEMORYMGMT, pMesProt->getDefaultFuncDone()));

    /* Load flash contents the usual way */
    memMgmtModeReadFlash(jobs, false, [](QVariant) {}, false, true, true);

    connect(jobs, &AsyncJobs::finished, [this, cb](const QByteArray &data)
    {
        Q_UNUSED(data);

        /* List constructor */
        QList<QVariantMap> list;

        /* Get file names */
        for (auto &i: dataNodes)
        {
            QVariantMap item;
            item.insert("name", i->getService());
            item.insert("size", i->getEncDataSize());
            list.append(item);
        }

        /* Clean vars, exit mmm */
        exitMemMgmtMode(false);
        cleanMMMVars();

        /* Callback */
        cb(true, list);
    });

    connect(jobs, &AsyncJobs::failed, [this, cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Setting device in MMM failed";
        exitMemMgmtMode(false);
        cb(false, QList<QVariantMap>());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::resetSmartCard(MessageHandlerCb cb)
{
    AsyncJobs *jobs = new AsyncJobs("Reseting smart card...", this);

    jobs->append(new MPCommandJob(this, MPCmd::RESET_CARD, pMesProt->getDefaultFuncDone()));

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Reseting smart card failed";
        cb(false, "");
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::lockDevice(const MessageHandlerCb &cb)
{
    auto *jobs = new AsyncJobs("Locking the device", this);

    // Currently the USB device doesn't return the correct value on success on 0xD9 (a bug!).
    const auto afterFn = [](const QByteArray &, bool &) -> bool { return true; };
    jobs->append(new MPCommandJob(this, MPCmd::LOCK_DEVICE, afterFn));

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Failed to lock device!";
        cb(false, {});
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::informLocked(const MessageHandlerCb &cb)
{
    auto *jobs = new AsyncJobs("Inform device about computer locked", this);

    const auto afterFn = [](const QByteArray &, bool &) -> bool { return true; };
    jobs->append(new MPCommandJob(this, MPCmd::INFORM_LOCKED, afterFn));

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Failed to inform locked!";
        cb(false, {});
    });

    jobsQueue.prepend(jobs);
    runAndDequeueJobs();
}

void MPDevice::informUnlocked(const MessageHandlerCb &cb)
{
    auto *jobs = new AsyncJobs("Inform device about computer unlocked", this);

    const auto afterFn = [](const QByteArray &, bool &) -> bool { return true; };
    jobs->append(new MPCommandJob(this, MPCmd::INFORM_UNLOCKED, afterFn));

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Failed to inform unlocked!";
        cb(false, {});
    });

    jobsQueue.prepend(jobs);
    runAndDequeueJobs();
}

void MPDevice::getAvailableUsers(const MessageHandlerCb &cb)
{
    auto *jobs = new AsyncJobs("Get Available User", this);

    jobs->append(new MPCommandJob(this, MPCmd::GET_AVAILABLE_USERS,
                      [this, cb](const QByteArray& data, bool &)
                        {
                            const auto availableUsers = pMesProt->getFirstPayloadByte(data);
                            qDebug() << "Available users: " << availableUsers;
                            cb(true, QString::number(availableUsers));
                            return true;
                        }
                      ));

    connect(jobs, &AsyncJobs::failed, [cb](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Failed to get available users!";
        cb(false, "");
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

MPDeviceBleImpl *MPDevice::ble() const
{
    return bleImpl;
}

DeviceSettings* MPDevice::settings() const
{
    return pSettings;
}

IMessageProtocol *MPDevice::getMesProt() const
{
    return pMesProt;
}
