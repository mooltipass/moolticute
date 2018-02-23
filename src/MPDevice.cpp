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

const QRegularExpression regVersion("v([0-9]+)\\.([0-9]+)(.*)");

MPDevice::MPDevice(QObject *parent):
    QObject(parent)
{
    set_status(Common::UnknownStatus);
    set_memMgmtMode(false); //by default device is not in MMM

    statusTimer = new QTimer(this);
    statusTimer->start(500);
    connect(statusTimer, &QTimer::timeout, [=]()
    {
        //Do not interfer with any other operation by sending a MOOLTIPASS_STATUS command
        if (commandQueue.size() > 0)
            return;

        sendData(MPCmd::MOOLTIPASS_STATUS, [=](bool success, const QByteArray &data, bool &)
        {
            if (!success)
                return;

            /* Map status from received val */
            Common::MPStatus s = (Common::MPStatus)data.at(2);
            Common::MPStatus prevStatus = get_status();

            /* Trigger on status change */
            if (s != prevStatus)
            {
                qDebug() << "received MPCmd::MOOLTIPASS_STATUS: " << (int)data.at(2);

                /* Update status */
                set_status(s);

                if (prevStatus == Common::UnknownStatus)
                {
                    /* First start: load parameters */
                    QTimer::singleShot(10, [=]()
                    {
                        loadParameters();
                        setCurrentDate();
                    });
                }

                if ((s == Common::Unlocked) || (s == Common::UnkownSmartcad))
                {
                    QTimer::singleShot(20, [=]()
                    {
                        getCurrentCardCPZ();
                    });
                }
                else
                {
                    filesCache.resetState();
                }

                if (s == Common::Unlocked)
                {
                    /* If v1.2 firmware, query user change number */
                    QTimer::singleShot(50, [=]()
                    {
                        if (isFw12())
                        {
                            qInfo() << "Firmware above v1.2, requesting change numbers";
                            getChangeNumbers();
                        }
                        else
                            qInfo() << "Firmware below v1.2, do not request change numbers";
                    });
                }
            }
        });
    });

    connect(this, SIGNAL(platformDataRead(QByteArray)), this, SLOT(newDataRead(QByteArray)));

//    connect(this, SIGNAL(platformFailed()), this, SLOT(commandFailed()));

    QTimer::singleShot(100, [this]() { exitMemMgmtMode(false); });
}

MPDevice::~MPDevice()
{
    filesCache.resetState();
}

void MPDevice::sendData(MPCmd::Command c, const QByteArray &data, quint32 timeout, MPCommandCb cb, bool checkReturn)
{
    MPCommand cmd;

    // Prepare MP packet
    cmd.data.append(data.size());
    cmd.data.append(c);
    cmd.data.append(data);
    cmd.cb = std::move(cb);
    cmd.checkReturn = checkReturn;
    cmd.retries_done = 0;
    cmd.sent_ts = QDateTime::currentMSecsSinceEpoch();

    cmd.timerTimeout = new QTimer(this);
    connect(cmd.timerTimeout, &QTimer::timeout, [=]()
    {
        commandQueue.head().retry--;

        if (commandQueue.head().retry > 0)
        {
            qDebug() << "> Retry command: " << MPCmd::printCmd(commandQueue.head().data);
            commandQueue.head().sent_ts = QDateTime::currentMSecsSinceEpoch();
            commandQueue.head().timerTimeout->start(); //restart timer
            commandQueue.head().retries_done++;
            platformWrite(commandQueue.head().data);
        }
        else
        {
            //Failed after all retry
            MPCommand currentCmd = commandQueue.head();
            delete currentCmd.timerTimeout;

            qWarning() << "> Retry command: " << MPCmd::printCmd(commandQueue.head().data) << " has failed too many times. Give up.";

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
            timeout += get_userInteractionTimeout() * 1000;
    }
    cmd.timerTimeout->setInterval(timeout);

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

    if (MPCmd::from(data[1]) == MPCmd::DEBUG)
    {
        qWarning() << data;
        return;
    }

    if (commandQueue.isEmpty())
    {
        qWarning() << "Command queue is empty!";
        qWarning() << "Packet data " << " size:" << (quint8)data[0] << " data:" << data;
        return;
    }

    MPCommand currentCmd = commandQueue.head();

    // First if: Resend the command, if device ask for retrying
    // Second if: Special case: if command check was requested but the device returned a mooltipass status (user entering his PIN), resend packet
    if ((MPCmd::from(data.at(MP_CMD_FIELD_INDEX)) == MPCmd::PLEASE_RETRY) ||
        (currentCmd.checkReturn &&
        MPCmd::from(currentCmd.data.at(MP_CMD_FIELD_INDEX)) != MPCmd::MOOLTIPASS_STATUS &&
        MPCmd::from(data.at(MP_CMD_FIELD_INDEX)) == MPCmd::MOOLTIPASS_STATUS &&
        (data.at(MP_PAYLOAD_FIELD_INDEX) & MP_UNLOCKING_SCREEN_BITMASK) != 0))
    {
        /* Stop timeout timer */
        commandQueue.head().timerTimeout->stop();

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
            qDebug() << MPCmd::printCmd(data) << " was received for a packet that was sent due to a timeout, not resending";
        }
        else
        {
            qDebug() << MPCmd::printCmd(data) << " received, resending command " << MPCmd::printCmd(commandQueue.head().data);
            QTimer *timer = new QTimer(this);
            connect(timer, &QTimer::timeout, [this, timer]()
            {
                timer->stop();
                timer->deleteLater();
                platformWrite(commandQueue.head().data);
                commandQueue.head().timerTimeout->start(); //restart timer
            });
            timer->start(300);
        }
        return;
    }

    //Only check returned command if it was asked
    //If the returned command does not match, fail
    if (currentCmd.checkReturn &&
        data[MP_CMD_FIELD_INDEX] != currentCmd.data[MP_CMD_FIELD_INDEX])
    {
        qWarning() << "Wrong answer received: " << MPCmd::printCmd(data)
                   << " for command: " << MPCmd::printCmd(currentCmd.data);
        return;
    }

//    qDebug() << "Received answer:" << MPCmd::printCmd(data);

    bool done = true;
    currentCmd.cb(true, data, done);
    delete currentCmd.timerTimeout;
    commandQueue.head().timerTimeout = nullptr;

    if (done)
    {
        commandQueue.dequeue();
        sendDataDequeue();
    }
}

void MPDevice::sendDataDequeue()
{
    if (commandQueue.isEmpty())
        return;

    MPCommand &currentCmd = commandQueue.head();
    currentCmd.running = true;

#ifdef DEV_DEBUG
    qDebug() << "Platform send command: " << MPCmd::printCmd(currentCmd.data);

    auto toHex = [](quint8 b) -> QString { return QString("0x%1").arg((quint8)b, 2, 16, QChar('0')); };
    QString a = "[";
    for (int i = 0;i < currentCmd.data.size();i++)
    {
        a += toHex((quint8)currentCmd.data.at(i));
        if (i < currentCmd.data.size() - 1) a += ", ";
    }
    a += "]";

    qDebug() << "Full packet: " << a;
#endif

    // send data with platform code
    platformWrite(currentCmd.data);

    currentCmd.timerTimeout->start();
}

void MPDevice::runAndDequeueJobs()
{
    if (jobsQueue.isEmpty() || currentJobs)
        return;

    currentJobs = jobsQueue.dequeue();

    connect(currentJobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        currentJobs = nullptr;
        runAndDequeueJobs();
    });
    connect(currentJobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        currentJobs = nullptr;
        runAndDequeueJobs();
    });

    currentJobs->start();
}

void MPDevice::updateFilesCache()
{
    getStoredFiles([=](bool success, QList<QVariantMap> files)
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

bool MPDevice::isJobsQueueBusy()
{
    return currentJobs;
}

void MPDevice::loadParameters()
{
    readingParams = true;
    AsyncJobs *jobs = new AsyncJobs(
                          "Loading device parameters",
                          this);

    jobs->append(new MPCommandJob(this,
                                  MPCmd::VERSION,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received MP version FLASH size: " << (quint8)data.at(2) << "Mb";
        QString hw = QString(data.mid(3, (quint8)data.at(0) - 2));
        qDebug() << "received MP version hw: " << hw;
        set_flashMbSize((quint8)data.at(2));
        set_hwVersion(hw);

        QRegularExpressionMatchIterator i = regVersion.globalMatch(hw);
        if (i.hasNext())
        {
            QRegularExpressionMatch match = i.next();
            int v = match.captured(1).toInt() * 10 +
                    match.captured(2).toInt();
            isFw12Flag = v >= 12;
            isMiniFlag = match.captured(3) == "_mini";
        }

        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::KEYBOARD_LAYOUT_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received language: " << (quint8)data.at(2);
        set_keyboardLayout((quint8)data.at(2));
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::LOCK_TIMEOUT_ENABLE_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received lock timeout enable: " << (quint8)data.at(2);
        set_lockTimeoutEnabled(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::LOCK_TIMEOUT_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received lock timeout: " << (quint8)data.at(2);
        set_lockTimeout((quint8)data.at(2));
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::SCREENSAVER_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received screensaver: " << (quint8)data.at(2);
        set_screensaver(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::USER_REQ_CANCEL_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received userRequestCancel: " << (quint8)data.at(2);
        set_userRequestCancel(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::USER_INTER_TIMEOUT_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received userInteractionTimeout: " << (quint8)data.at(2);
        set_userInteractionTimeout((quint8)data.at(2));
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::FLASH_SCREEN_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received flashScreen: " << (quint8)data.at(2);
        set_flashScreen(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::OFFLINE_MODE_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received offlineMode: " << (quint8)data.at(2);
        set_offlineMode(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::TUTORIAL_BOOL_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received tutorialEnabled: " << (quint8)data.at(2);
        set_tutorialEnabled(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::MINI_OLED_CONTRAST_CURRENT_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received screenBrightness: " << (quint8)data.at(2);
        set_screenBrightness((quint8)data.at(2));
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::MINI_KNOCK_DETECT_ENABLE_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received set_knockEnabled: " << (quint8)data.at(2);
        set_knockEnabled(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::MINI_KNOCK_THRES_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received knockSensitivity: " << (quint8)data.at(2);
        int v = 1;
        if (data.at(2) == 11) v = 0;
        else if (data.at(2) == 5) v = 2;
        set_knockSensitivity(v);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::RANDOM_INIT_PIN_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received randomStartingPin: " << (quint8)data.at(2);
        set_randomStartingPin(data.at(2) != 0);
        return true;
    }));


    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::HASH_DISPLAY_FEATURE_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received hashDisplay: " << (quint8)data.at(2);
        set_hashDisplay(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::LOCK_UNLOCK_FEATURE_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received lockUnlockMode: " << (quint8)data.at(2);
        set_lockUnlockMode(data.at(2));
        return true;
    }));


    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::KEY_AFTER_LOGIN_SEND_BOOL_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received key after login send enabled: " << (quint8)data.at(2);
        set_keyAfterLoginSendEnable(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::KEY_AFTER_LOGIN_SEND_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received key after login send " << (quint8)data.at(2);
        set_keyAfterLoginSend(data.at(2));
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::KEY_AFTER_PASS_SEND_BOOL_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received key after pass send enabled: " << (quint8)data.at(2);
        set_keyAfterPassSendEnable(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::KEY_AFTER_PASS_SEND_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received key after pass send " << (quint8)data.at(2);
        set_keyAfterPassSend(data.at(2));
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::DELAY_AFTER_KEY_ENTRY_BOOL_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received delay after key entry enabled: " << (quint8)data.at(2);
        set_delayAfterKeyEntryEnable(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::DELAY_AFTER_KEY_ENTRY_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received delay after key entry " << (quint8)data.at(2);
        set_delayAfterKeyEntry(data.at(2));
        return true;
    }));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &data)
    {
        Q_UNUSED(data);
        //data is last result
        //all jobs finished success
        qInfo() << "Finished loading device options";

        if (isFw12() && isMini())
        {
            qInfo() << "Mini firmware above v1.2, requesting serial number";

            AsyncJobs* v12jobs = new AsyncJobs("Loading device serial number", this);

            /* Query serial number */
            v12jobs->append(new MPCommandJob(this,
                                          MPCmd::GET_SERIAL,
                                          [=](const QByteArray &data, bool &) -> bool
            {
                set_serialNumber(((quint8)data[MP_PAYLOAD_FIELD_INDEX+3]) +
                        ((quint32)((quint8)data[MP_PAYLOAD_FIELD_INDEX+2]) << 8) +
                        ((quint32)((quint8)data[MP_PAYLOAD_FIELD_INDEX+1]) << 16) +
                        ((quint32)((quint8)data[MP_PAYLOAD_FIELD_INDEX+0]) << 24));
                qDebug() << "Mooltipass Mini serial number:" << get_serialNumber();
                return true;
            }));

            connect(v12jobs, &AsyncJobs::finished, [=](const QByteArray &data)
            {
                Q_UNUSED(data);
                //data is last result
                //all jobs finished success
                readingParams = false;
                qInfo() << "Finished loading Mini serial number";
            });

            connect(v12jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
            {
                Q_UNUSED(failedJob);
                qCritical() << "Loading Mini serial number failed";
                readingParams = false;
                loadParameters(); // memory: does it get "piled on?"
            });
            jobsQueue.enqueue(v12jobs);
            runAndDequeueJobs();
        }
    });

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Loading option failed";
        readingParams = false;
        loadParameters(); // memory: does it get "piled on?"
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::updateParam(MPParams::Param param, int val)
{
    QMetaEnum m = QMetaEnum::fromType<MPParams::Param>();
    QString logInf = QStringLiteral("Updating %1 param: %2").arg(m.valueToKey(param)).arg(val);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    QByteArray ba;
    ba.append((quint8)param);
    ba.append((quint8)val);

    jobs->append(new MPCommandJob(this, MPCmd::SET_MOOLTIPASS_PARM, ba, MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << param << " param updated with success";
    });
    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed to change " << param;
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::updateParam(MPParams::Param param, bool en)
{
    updateParam(param, (int)en);
}

void MPDevice::updateKeyboardLayout(int lang)
{
    updateParam(MPParams::KEYBOARD_LAYOUT_PARAM, lang);
}

void MPDevice::updateLockTimeoutEnabled(bool en)
{
    updateParam(MPParams::LOCK_TIMEOUT_ENABLE_PARAM, en);
}

void MPDevice::updateLockTimeout(int timeout)
{
    if (timeout < 0) timeout = 0;
    if (timeout > 0xFF) timeout = 0xFF;
    updateParam(MPParams::LOCK_TIMEOUT_PARAM, timeout);
}

void MPDevice::updateScreensaver(bool en)
{
    updateParam(MPParams::SCREENSAVER_PARAM, en);
  }

void MPDevice::updateUserRequestCancel(bool en)
{   
    updateParam(MPParams::USER_REQ_CANCEL_PARAM, en);
}

void MPDevice::updateUserInteractionTimeout(int timeout)
{   
    if (timeout < 0) timeout = 0;
    if (timeout > 0xFF) timeout = 0xFF;
    updateParam(MPParams::USER_INTER_TIMEOUT_PARAM, timeout);
}

void MPDevice::updateFlashScreen(bool en)
{
    updateParam(MPParams::FLASH_SCREEN_PARAM, en);
}

void MPDevice::updateOfflineMode(bool en)
{
    updateParam(MPParams::OFFLINE_MODE_PARAM, en);
}

void MPDevice::updateTutorialEnabled(bool en)
{
    updateParam(MPParams::TUTORIAL_BOOL_PARAM, en);
}

void MPDevice::updateScreenBrightness(int bval) //In percent
{
    updateParam(MPParams::MINI_OLED_CONTRAST_CURRENT_PARAM, bval);
}

void MPDevice::updateKnockEnabled(bool en)
{
    updateParam(MPParams::MINI_KNOCK_DETECT_ENABLE_PARAM, en);
}

void MPDevice::updateKeyAfterLoginSendEnable(bool en)
{
    updateParam(MPParams::KEY_AFTER_LOGIN_SEND_BOOL_PARAM, en);
}

void MPDevice::updateKeyAfterLoginSend(int value)
{
    updateParam(MPParams::KEY_AFTER_LOGIN_SEND_PARAM, value);
}

void MPDevice::updateKeyAfterPassSendEnable(bool en)
{
     updateParam(MPParams::KEY_AFTER_PASS_SEND_BOOL_PARAM, en);
}

void MPDevice::updateKeyAfterPassSend(int value)
{
    updateParam(MPParams::KEY_AFTER_PASS_SEND_PARAM, value);
}

void MPDevice::updateDelayAfterKeyEntryEnable(bool en)
{
    updateParam(MPParams::DELAY_AFTER_KEY_ENTRY_BOOL_PARAM, en);
}

void MPDevice::updateDelayAfterKeyEntry(int val)
{
    updateParam(MPParams::DELAY_AFTER_KEY_ENTRY_PARAM, val);
}

void MPDevice::updateKnockSensitivity(int s) // 0-low, 1-medium, 2-high
{
    quint8 v = 8;
    if (s == 0) v = 11;
    else if (s == 2) v = 5;
    updateParam(MPParams::MINI_KNOCK_THRES_PARAM, v);
}

void MPDevice::updateRandomStartingPin(bool en)
{
    updateParam(MPParams::RANDOM_INIT_PIN_PARAM, en);
}

void MPDevice::updateHashDisplay(bool en)
{
    updateParam(MPParams::HASH_DISPLAY_FEATURE_PARAM, en);
}

void MPDevice::updateLockUnlockMode(int val)
{
    updateParam(MPParams::LOCK_UNLOCK_FEATURE_PARAM, val);
}

void MPDevice::memMgmtModeReadFlash(AsyncJobs *jobs, bool fullScan,
                                    MPDeviceProgressCb cbProgress,bool getCreds,
                                    bool getData, bool getDataChilds)
{
    /* For when the MMM is left */
    cleanMMMVars();

    /* Get CTR value */
    jobs->append(new MPCommandJob(this, MPCmd::GET_CTRVALUE,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[MP_LEN_FIELD_INDEX] == 1)
        {
            /* Received one byte as answer: command fail */
            jobs->setCurrentJobError("Mooltipass refused to send us a CTR packet");
            qCritical() << "Get CTR value: couldn't get answer";
            return false;
        }
        else
        {
            ctrValue = data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]);
            ctrValueClone = data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]);
            qDebug() << "CTR value:" << ctrValue.toHex();

            progressTotal = 100 + MOOLTIPASS_FAV_MAX;
            progressCurrent = 0;
            return true;
        }
    }));

    /* Get CPZ and CTR values */
    auto cpzJob = new MPCommandJob(this, MPCmd::GET_CARD_CPZ_CTR,
                                  [=](const QByteArray &data, bool &done) -> bool
    {
        /* The Mooltipass answers with CPZ CTR packets containing the CPZ_CTR values, and then a final MPCmd::GET_CARD_CPZ_CTR packet */
        if ((quint8)data[1] == MPCmd::CARD_CPZ_CTR_PACKET)
        {
            QByteArray cpz = data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]);
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
        else if((quint8)data[1] == MPCmd::GET_CARD_CPZ_CTR)
        {
            /* Received packet indicating we received all CPZ CTR packets */
            qDebug() << "All CPZ CTR packets received";
            return true;
        }
        else
        {
            qCritical() << "Get CPZ CTR: wrong command received as answer:" << MPCmd::printCmd(data);
            jobs->setCurrentJobError("Get CPZ/CTR: Mooltipass sent an answer packet with a different command ID");
            return false;
        }
    });
    cpzJob->setReturnCheck(false); //disable return command check
    jobs->append(cpzJob);

    /* Get favorites */
    for (int i = 0; i<MOOLTIPASS_FAV_MAX; i++)
    {
        jobs->append(new MPCommandJob(this, MPCmd::GET_FAVORITE,
                                      QByteArray(1, (quint8)i),
                                      [=](const QByteArray &, QByteArray &) -> bool
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
                                      [=](const QByteArray &data, bool &) -> bool
        {
            if (data[MP_LEN_FIELD_INDEX] == 1)
            {
                /* Received one byte as answer: command fail */
                jobs->setCurrentJobError("Mooltipass refused to send us favorites");
                qCritical() << "Get favorite: couldn't get answer";
                return false;
            }
            else
            {
                /* Append favorite to list */
                qDebug() << "Favorite" << i << ": parent address:" << data.mid(MP_PAYLOAD_FIELD_INDEX, 2).toHex() << ", child address:" << data.mid(MP_PAYLOAD_FIELD_INDEX+2, 2).toHex();
                favoritesAddrs.append(data.mid(MP_PAYLOAD_FIELD_INDEX, MOOLTIPASS_ADDRESS_SIZE));
                favoritesAddrsClone.append(data.mid(MP_PAYLOAD_FIELD_INDEX, MOOLTIPASS_ADDRESS_SIZE));

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

    if (getCreds)
    {
        /* Get parent node start address */
        jobs->append(new MPCommandJob(this, MPCmd::GET_STARTING_PARENT,
                                      [=](const QByteArray &data, bool &) -> bool
        {
            if (data[MP_LEN_FIELD_INDEX] == 1)
            {
                /* Received one byte as answer: command fail */
                jobs->setCurrentJobError("Mooltipass refused to send us starting parent");
                qCritical() << "Get start node addr: couldn't get answer";
                return false;
            }
            else
            {
                startNode = data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]);
                startNodeClone = data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]);
                qDebug() << "Start node addr:" << startNode.toHex();

                //if parent address is not null, load nodes
                if (startNode != MPNode::EmptyAddress)
                {
                    qInfo() << "Loading parent nodes...";
                    if (!fullScan)
                    {
                        /* Traverse the flash by following the linked list */
                        loadLoginNode(jobs, startNode, cbProgress);
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

                return true;
            }
        }));
    }

    if (getData)
    {
        //Get parent data node start address
        jobs->append(new MPCommandJob(this, MPCmd::GET_DN_START_PARENT,
                                      [=](const QByteArray &data, bool &) -> bool
        {

            if (data[MP_LEN_FIELD_INDEX] == 1)
            {
                /* Received one byte as answer: command fail */
                jobs->setCurrentJobError("Mooltipass refused to send us data starting parent");
                qCritical() << "Get data start node addr: couldn't get answer";
                return false;
            }
            else
            {
                startDataNode = data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]);
                startDataNodeClone = data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]);
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
                                MPDeviceProgressCb cbProgress,
                                std::function<void(bool success, int errCode, QString errMsg)> cb)
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
    auto startMmmJob = new MPCommandJob(this, MPCmd::START_MEMORYMGMT, MPCommandJob::defaultCheckRet);
    startMmmJob->setTimeout(15000); //We need a big timeout here in case user enter a wrong pin code
    jobs->append(startMmmJob);

    /* Load flash contents the usual way */
    memMgmtModeReadFlash(jobs, false, cbProgress, !wantData, wantData, true);

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &data)
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

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
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
    if(get_flashMbSize() >= 16)
    {
        return 256*get_flashMbSize();
    }
    else
    {
        return 512*get_flashMbSize();
    }
}

quint16 MPDevice::getFlashPageFromAddress(const QByteArray &address)
{
    return (((quint16)address[1] << 5) & 0x1FE0) | (((quint16)address[0] >> 3) & 0x001F);
}

quint8 MPDevice::getNodeIdFromAddress(const QByteArray &address)
{
    return (quint8)address[0] & 0x07;
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

void MPDevice::loadSingleNodeAndScan(AsyncJobs *jobs, const QByteArray &address, MPDeviceProgressCb cbProgress)
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
    MPNode *pnodeClone = new MPNode(this, address);
    MPNode *pnode = new MPNode(this, address);

    /* Send read node command, expecting 3 packets or 1 depending on if we're allowed to read a block*/
    jobs->append(new MPCommandJob(this, MPCmd::READ_FLASH_NODE,
                                  address,
                                  [=](const QByteArray &data, bool &done) -> bool
    {
        if (data[MP_LEN_FIELD_INDEX] == 1)
        {
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
            pnode->appendData(data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]));
            pnodeClone->appendData(data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]));

            // Continue to read data until the node is fully received
            if (!pnode->isDataLengthValid())
            {
                done = false;
            }
            else
            {
                // Node is loaded
                if (!pnode->isValid())
                {
                    //qDebug() << address.toHex() << ": empty node loaded";

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

void MPDevice::loadLoginNode(AsyncJobs *jobs, const QByteArray &address, MPDeviceProgressCb cbProgress)
{    
    qDebug() << "Loading cred parent node at address: " << address.toHex();

    /* Create new parent node, append to list */
    MPNode *pnode = new MPNode(this, address);
    loginNodes.append(pnode);
    MPNode *pnodeClone = new MPNode(this, address);
    loginNodesClone.append(pnodeClone);

    /* Send read node command, expecting 3 packets */
    jobs->append(new MPCommandJob(this, MPCmd::READ_FLASH_NODE,
                                  address,
                                  [=](const QByteArray &data, bool &done) -> bool
    {
        if (data[MP_LEN_FIELD_INDEX] == 1)
        {
            /* Received one byte as answer: command fail */
            jobs->setCurrentJobError("Couldn't read parent node, card removed or database corrupted");
            qCritical() << "Get node: couldn't get answer";
            return false;
        }
        else
        {
            /* Append received data to node data */
            pnode->appendData(data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]));
            pnodeClone->appendData(data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]));

            //Continue to read data until the node is fully received
            if (!pnode->isDataLengthValid())
            {
                done = false;
            }
            else
            {                
                QString srv = pnode->getService();
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
                qDebug() << address.toHex() << ": parent node loaded:" << pnode->getService();

                QVariantMap data = {
                    {"total", progressTotal},
                    {"current", progressCurrent},
                    {"msg", "Loading credentials for %1" },
                    {"msg_args", QVariantList({pnode->getService()})}
                };
                cbProgress(data);

                if (pnode->getStartChildAddress() != MPNode::EmptyAddress)
                {
                    qDebug() << pnode->getService() << ": loading child nodes...";
                    loadLoginChildNode(jobs, pnode, pnodeClone, pnode->getStartChildAddress());
                }
                else
                {
                    qDebug() << "Parent does not have childs.";
                }

                //Load next parent
                if (pnode->getNextParentAddress() != MPNode::EmptyAddress)
                {
                    loadLoginNode(jobs, pnode->getNextParentAddress(), cbProgress);
                }
            }

            return true;
        }
    }));
}

void MPDevice::loadLoginChildNode(AsyncJobs *jobs, MPNode *parent, MPNode *parentClone, const QByteArray &address)
{
    qDebug() << "Loading cred child node at address:" << address.toHex();

    /* Create empty child node and add it to the list */
    MPNode *cnode = new MPNode(this, address);
    loginChildNodes.append(cnode);
    parent->appendChild(cnode);
    MPNode *cnodeClone = new MPNode(this, address);
    loginChildNodesClone.append(cnodeClone);
    parentClone->appendChild(cnodeClone);

    /* Query node */
    jobs->prepend(new MPCommandJob(this, MPCmd::READ_FLASH_NODE,
                                  address,
                                  [=](const QByteArray &data, bool &done) -> bool
    {
        if (data[MP_LEN_FIELD_INDEX] == 1)
        {
            /* Received one byte as answer: command fail */
            jobs->setCurrentJobError("Couldn't read child node, card removed or database corrupted");
            qCritical() << "Get child node: couldn't get answer";
            return false;
        }
        else
        {
            /* Append received data to node data */
            cnode->appendData(data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]));
            cnodeClone->appendData(data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]));

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
                    loadLoginChildNode(jobs, parent, parentClone, cnode->getNextChildAddress());
                }
            }

            return true;
        }
    }));
}

void MPDevice::loadDataNode(AsyncJobs *jobs, const QByteArray &address, bool load_childs, MPDeviceProgressCb cbProgress)
{
    MPNode *pnode = new MPNode(this, address);
    dataNodes.append(pnode);
    MPNode *pnodeClone = new MPNode(this, address);
    dataNodesClone.append(pnodeClone);

    qDebug() << "Loading data parent node at address: " << address.toHex();

    jobs->append(new MPCommandJob(this, MPCmd::READ_FLASH_NODE,
                                  address,
                                  [=](const QByteArray &data, bool &done) -> bool
    {
        if (data[MP_LEN_FIELD_INDEX] == 1)
        {
            /* Received one byte as answer: command fail */
            jobs->setCurrentJobError("Couldn't read data node, card removed or database corrupted");
            qCritical() << "Get data node: couldn't get answer";
            return false;
        }

        pnode->appendData(data.mid(2, data[0]));
        pnodeClone->appendData(data.mid(2, data[0]));

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

void MPDevice::loadDataChildNode(AsyncJobs *jobs, MPNode *parent, MPNode *parentClone, const QByteArray &address, MPDeviceProgressCb cbProgress, quint32 nbBytesFetched)
{
    MPNode *cnode = new MPNode(this, address);
    parent->appendChildData(cnode);
    dataChildNodes.append(cnode);
    MPNode *cnodeClone = new MPNode(this, address);
    parentClone->appendChildData(cnodeClone);
    dataChildNodesClone.append(cnodeClone);

    qDebug() << "Loading data child node at address: " << address.toHex();

    jobs->prepend(new MPCommandJob(this, MPCmd::READ_FLASH_NODE,
                                  address,
                                  [=](const QByteArray &data, bool &done) -> bool
    {
        if (data[MP_LEN_FIELD_INDEX] == 1)
        {
            /* Received one byte as answer: command fail */
            jobs->setCurrentJobError("Couldn't read data child node, card removed or database corrupted");
            qCritical() << "Get data child node: couldn't get answer";
            return false;
        }

        cnode->appendData(data.mid(2, data[0]));
        cnodeClone->appendData(data.mid(2, data[0]));

        //Continue to read data until the node is fully received
        if (!cnode->isValid())
        {
            done = false;
        }
        else
        {
            //Node is loaded
            qDebug() << "Child data node loaded";

            QVariantMap data = {
                {"total", -1},
                {"current", 0},
                {"msg", "Loading data for %1: %2 encrypted bytes read" },
                {"msg_args", QVariantList({parent->getService(), nbBytesFetched + MP_NODE_DATA_ENC_SIZE})}
            };
            cbProgress(data);

            //Load next child
            if (cnode->getNextChildDataAddress() != MPNode::EmptyAddress)
            {
                loadDataChildNode(jobs, parent, parentClone, cnode->getNextChildDataAddress(), cbProgress, nbBytesFetched + MP_NODE_DATA_ENC_SIZE);
            }
            else
            {
                parent->setEncDataSize(nbBytesFetched + MP_NODE_DATA_ENC_SIZE);
                parentClone->setEncDataSize(nbBytesFetched + MP_NODE_DATA_ENC_SIZE);

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
            MPNode* curNode = findNodeWithAddressInList(loginChildNodes, curChildNodeAddr);

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
MPNode *MPDevice::findNodeWithAddressInList(QList<MPNode *> list, const QByteArray &address, const quint32 virt_addr)
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
MPNode *MPDevice::findNodeWithNameInList(QList<MPNode *> list, const QString& name, bool isParent)
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
MPNode *MPDevice::findNodeWithLoginWithGivenParentInList(QList<MPNode *> list,  MPNode *parent, const QString& name)
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
MPNode *MPDevice::findNodeWithAddressWithGivenParentInList(QList<MPNode *> list,  MPNode *parent, const QByteArray &address, const quint32 virt_addr)
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
MPNode *MPDevice::findNodeWithServiceInList(const QString &service)
{
    auto it = std::find_if(loginNodes.begin(), loginNodes.end(), [&service](const MPNode *const node)
    {
        return node->getService() == service;
    });

    return it == loginNodes.end()?nullptr:*it;
}

/* Find a node inside the child list given his name */
MPNode *MPDevice::findNodeWithLoginInList(const QString &login)
{
    auto it = std::find_if(loginChildNodes.begin(), loginChildNodes.end(), [&login](const MPNode *const node)
    {
        return node->getLogin() == login;
    });

    return it == loginChildNodes.end()?nullptr:*it;
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
    tempParentAddress = startNode;
    tempVirtualParentAddress = virtualStartNode;

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

void MPDevice::detagPointedNodes(void)
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

/* Follow the chain to tag pointed nodes (useful when doing integrity check when we are getting everything we can) */
bool MPDevice::tagPointedNodes(bool tagCredentials, bool tagData, bool repairAllowed)
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

    /* first, detag all nodes */
    detagPointedNodes();

    if (tagCredentials)
    {
        /* start with start node (duh) */
        tempParentAddress = startNode;
        tempVirtualParentAddress = virtualStartNode;

        /* Loop through the parent nodes */
        while ((tempParentAddress != MPNode::EmptyAddress) || (tempParentAddress.isNull() && tempVirtualParentAddress != 0))
        {
            /* Get pointer to next parent node */
            tempNextParentNodePt = findNodeWithAddressInList(loginNodes, tempParentAddress, tempVirtualParentAddress);

            /* Check that we could actually find it */
            if (!tempNextParentNodePt)
            {
                qCritical() << "tagPointedNodes: couldn't find parent node with address" << tempParentAddress.toHex() << "in our list";

                if (repairAllowed)
                {
                    if ((!tempParentAddress.isNull() && tempParentAddress == startNode) || (tempParentAddress.isNull() && tempVirtualParentAddress == virtualStartNode))
                    {
                        /* start node is incorrect */
                        startNode = QByteArray(MPNode::EmptyAddress);
                        virtualStartNode = 0;
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
                    if ((!tempParentAddress.isNull() && tempParentAddress == startNode) || (tempParentAddress.isNull() && tempVirtualParentAddress == virtualStartNode))
                    {
                        /* start node is already tagged... how's that possible? */
                        startNode = QByteArray(MPNode::EmptyAddress);
                        virtualStartNode = 0;
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
                if ((!tempParentAddress.isNull() && tempParentAddress == startNode) || (tempParentAddress.isNull() && tempVirtualParentAddress == virtualStartNode))
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
                    tempNextChildNodePt = findNodeWithAddressInList(loginChildNodes, tempChildAddress, tempVirtualChildAddress);

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

    if (tagData)
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


bool MPDevice::addOrphanParentChildsToDB(MPNode *parentNodePt, bool isDataParent)
{
    quint32 cur_child_addr_v = parentNodePt->getStartChildVirtualAddress();
    QByteArray cur_child_addr = parentNodePt->getStartChildAddress();
    MPNode* prev_child_pt = nullptr;

    /* Tag nodes */
    tagPointedNodes(!isDataParent, isDataParent, false);

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
            cur_child_pt = findNodeWithAddressInList(loginChildNodes, cur_child_addr, cur_child_addr_v);
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
bool MPDevice::addOrphanParentToDB(MPNode *parentNodePt, bool isDataParent, bool addPossibleChildren)
{
    MPNode* prevNodePt = nullptr;
    MPNode* curNodePt = nullptr;
    quint32 curNodeAddrVirtual;
    QByteArray curNodeAddr;

    /* Tag nodes */
    if (!tagPointedNodes(!isDataParent, isDataParent, false))
    {
        qCritical() << "Can't add orphan parent to a corrupted DB, please run integrity check";
        return false;
    }

    /* Detag them */
    detagPointedNodes();

    /* Which list do we want to browse ? */
    if (isDataParent)
    {
        curNodeAddr = startDataNode;
        curNodeAddrVirtual = virtualDataStartNode;
    }
    else
    {
        curNodeAddr = startNode;
        curNodeAddrVirtual = virtualStartNode;
    }

    qInfo() << "Adding parent node" << parentNodePt->getService();

    if (parentNodePt->getPointedToCheck())
    {
        qCritical() << "addParentOrphan: parent node" << parentNodePt->getService() << "is already pointed to";
        tagPointedNodes(!isDataParent, isDataParent, false);
        if (addPossibleChildren)
        {
            addOrphanParentChildsToDB(parentNodePt, isDataParent);
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
                startNode = parentNodePt->getAddress();
                virtualStartNode = parentNodePt->getVirtualAddress();
            }

            /* Update prev/next fields */
            parentNodePt->setPreviousParentAddress(MPNode::EmptyAddress, 0);
            parentNodePt->setNextParentAddress(MPNode::EmptyAddress, 0);
            tagPointedNodes(!isDataParent, isDataParent, false);
            if (addPossibleChildren)
            {
                addOrphanParentChildsToDB(parentNodePt, isDataParent);
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
                curNodePt = findNodeWithAddressInList(loginNodes, curNodeAddr, curNodeAddrVirtual);
            }

            /* Check if we could find the parent */
            if (!curNodePt)
            {
                qCritical() << "Broken parent linked list, please run integrity check";
                tagPointedNodes(!isDataParent, isDataParent, false);
                return false;
            }
            else
            {
                /* Check for tagged to avoid loops */
                if (curNodePt->getPointedToCheck())
                {
                    qCritical() << "Linked list loop detected, please run integrity check";
                    tagPointedNodes(!isDataParent, isDataParent, false);
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
                            startNode = parentNodePt->getAddress();
                            virtualStartNode = parentNodePt->getVirtualAddress();
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
                    tagPointedNodes(!isDataParent, isDataParent, false);
                    if (addPossibleChildren)
                    {
                        addOrphanParentChildsToDB(parentNodePt, isDataParent);
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

        /* If we are here it means we need to take the last spot */
        qDebug() << "Adding parent node after" << prevNodePt->getService();
        prevNodePt->setNextParentAddress(parentNodePt->getAddress(), parentNodePt->getVirtualAddress());
        parentNodePt->setPreviousParentAddress(prevNodePt->getAddress(), prevNodePt->getVirtualAddress());
        parentNodePt->setNextParentAddress(MPNode::EmptyAddress);
        tagPointedNodes(!isDataParent, isDataParent, false);
        if (addPossibleChildren)
        {
            addOrphanParentChildsToDB(parentNodePt, isDataParent);
        }
        return true;
    }
}

MPNode* MPDevice::addNewServiceToDB(const QString &service)
{
    MPNode* tempNodePt;
    MPNode* newNodePt;

    qDebug() << "Creating new service" << service << "in DB";

    /* Does the service actually exist? */
    tempNodePt = findNodeWithServiceInList(service);
    if (tempNodePt)
    {
        qCritical() << "Service already exists.... dumbass!";
        return nullptr;
    }

    /* Increment new addresses counter */
    newAddressesNeededCounter += 1;

    /* Create new node with null address and virtual address set to our counter value */
    newNodePt = new MPNode(QByteArray(MP_NODE_SIZE, 0), this, QByteArray(), newAddressesNeededCounter);
    newNodePt->setType(MPNode::NodeParent);
    newNodePt->setService(service);

    /* Add node to list */
    loginNodes.append(newNodePt);
    addOrphanParentToDB(newNodePt, false, false);

    return newNodePt;
}

bool MPDevice::addChildToDB(MPNode* parentNodePt, MPNode* childNodePt)
{
    qInfo() << "Adding child " << childNodePt->getLogin() << " with parent " << parentNodePt->getService() << " to DB";
    MPNode* tempChildNodePt = nullptr;
    MPNode* tempNextChildNodePt;

    /* Detag nodes */
    detagPointedNodes();

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
        tagPointedNodes(true, false, false);
        return true;
    }

    /* browse through all the children to find the right slot */
    while ((tempChildAddress != MPNode::EmptyAddress) || (tempChildAddress.isNull() && tempVirtualChildAddress != 0))
    {
        /* Get pointer to the child node */
        tempNextChildNodePt = findNodeWithAddressInList(loginChildNodes, tempChildAddress, tempVirtualChildAddress);

        /* Check we could find child pointer */
        if (!tempNextChildNodePt)
        {
            qCritical() << "Broken child linked list, please run integrity check";
            tagPointedNodes(true, false, false);
            return false;
        }
        else
        {
            /* Check for tagged to avoid loops */
            if (tempNextChildNodePt->getPointedToCheck())
            {
                qCritical() << "Linked list loop detected, please run integrity check";
                tagPointedNodes(true, false, false);
                return false;
            }

            /* Tag node */
            tempNextChildNodePt->setPointedToCheck();

            if (tempNextChildNodePt->getLogin().compare(childNodePt->getLogin()) == 0)
            {
                qCritical() << "Can't add child node that has the exact same name!";
                tagPointedNodes(true, false, false);
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
                    tagPointedNodes(true, false, false);
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
                    tagPointedNodes(true, false, false);
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
    tagPointedNodes(true, false, false);
    return true;
}

bool MPDevice::removeEmptyParentFromDB(MPNode* parentNodePt, bool isDataParent)
{
    qDebug() << "Removing parent " << parentNodePt->getService() << " from DB, addr: " << parentNodePt->getAddress().toHex();
    MPNode* nextNodePt = nullptr;
    MPNode* prevNodePt = nullptr;
    MPNode* curNodePt = nullptr;
    quint32 curNodeAddrVirtual;
    QByteArray curNodeAddr;

    /* Tag nodes */
    if (!tagPointedNodes(!isDataParent, isDataParent, false))
    {
        qCritical() << "Can't remove parent to a corrupted DB, please run integrity check";
        return false;
    }

    /* Detag them */
    detagPointedNodes();

    /* Which list do we want to browse ? */
    if (isDataParent)
    {
        curNodeAddr = startDataNode;
        curNodeAddrVirtual = virtualDataStartNode;
    }
    else
    {
        curNodeAddr = startNode;
        curNodeAddrVirtual = virtualStartNode;
    }

    if (parentNodePt->getPointedToCheck())
    {
        qCritical() << "addParentOrphan: parent node" << parentNodePt->getService() << "is already pointed to";
        tagPointedNodes(!isDataParent, isDataParent, false);
        return true;
    }
    else
    {
        /* Check for Empty DB */
        if ((curNodeAddr == MPNode::EmptyAddress) || (curNodeAddr.isNull() && curNodeAddrVirtual == 0))
        {
            qCritical() << "Database is empty!";
            tagPointedNodes(!isDataParent, isDataParent, false);
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
                curNodePt = findNodeWithAddressInList(loginNodes, curNodeAddr, curNodeAddrVirtual);
            }

            /* Check if we could find the parent */
            if (!curNodePt)
            {
                qCritical() << "Broken parent linked list, please run integrity check";
                tagPointedNodes(!isDataParent, isDataParent, false);
                return false;
            }
            else
            {
                /* Check for tagged to avoid loops */
                if (curNodePt->getPointedToCheck())
                {
                    qCritical() << "Linked list loop detected, please run integrity check";
                    tagPointedNodes(!isDataParent, isDataParent, false);
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
                        tagPointedNodes(!isDataParent, isDataParent, false);
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
                            nextNodePt = findNodeWithAddressInList(loginNodes, parentNodePt->getNextParentAddress(), parentNodePt->getNextParentVirtualAddress());
                        }

                        if (!nextNodePt)
                        {
                            qCritical() << "Broken parent linked list, please run integrity check";
                            tagPointedNodes(!isDataParent, isDataParent, false);
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
                                startNode = MPNode::EmptyAddress;
                                virtualStartNode = 0;
                            }

                            /* Delete object */
                            if (isDataParent)
                            {
                                dataNodes.removeOne(parentNodePt);
                            }
                            else
                            {
                                loginNodes.removeOne(parentNodePt);
                            }
                            delete(parentNodePt);
                            tagPointedNodes(!isDataParent, isDataParent, false);
                            return true;
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
                                startNode = nextNodePt->getAddress();
                                virtualStartNode = nextNodePt->getVirtualAddress();
                            }
                            nextNodePt->setPreviousParentAddress(MPNode::EmptyAddress, 0);

                            /* Delete object */
                            if (isDataParent)
                            {
                                dataNodes.removeOne(parentNodePt);
                            }
                            else
                            {
                                loginNodes.removeOne(parentNodePt);
                            }
                            delete(parentNodePt);
                            tagPointedNodes(!isDataParent, isDataParent, false);
                            return true;
                        }
                    }
                    else
                    {
                        if (isLastNode)
                        {
                            /* Linked list ends there */
                            prevNodePt->setNextParentAddress(MPNode::EmptyAddress, 0);

                            /* Delete object */
                            if (isDataParent)
                            {
                                dataNodes.removeOne(parentNodePt);
                            }
                            else
                            {
                                loginNodes.removeOne(parentNodePt);
                            }
                            delete(parentNodePt);
                            tagPointedNodes(!isDataParent, isDataParent, false);
                            return true;
                        }
                        else
                        {
                            /* Link the chain together */
                            prevNodePt->setNextParentAddress(nextNodePt->getAddress(), nextNodePt->getVirtualAddress());
                            nextNodePt->setPreviousParentAddress(prevNodePt->getAddress(), prevNodePt->getVirtualAddress());

                            /* Delete object */
                            if (isDataParent)
                            {
                                dataNodes.removeOne(parentNodePt);
                            }
                            else
                            {
                                loginNodes.removeOne(parentNodePt);
                            }
                            delete(parentNodePt);
                            tagPointedNodes(!isDataParent, isDataParent, false);
                            return true;
                        }
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
        tagPointedNodes(!isDataParent, isDataParent, false);
        return false;
    }
}

bool MPDevice::removeChildFromDB(MPNode* parentNodePt, MPNode* childNodePt, bool deleteEmptyParent)
{
    qDebug() << "Removing child " << childNodePt->getLogin() << " with parent " << parentNodePt->getService() << " from DB";
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

    /* browse through all the children to find the right child */
    while ((tempChildAddress != MPNode::EmptyAddress) || (tempChildAddress.isNull() && tempVirtualChildAddress != 0))
    {
        /* Get pointer to the child node */
        tempNextChildNodePt = findNodeWithAddressInList(loginChildNodes, tempChildAddress, tempVirtualChildAddress);

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
                    tempNextChildNodePt = findNodeWithAddressInList(loginChildNodes, childNodePt->getNextChildAddress(), childNodePt->getNextChildVirtualAddress());

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

                        /* Delete possible fav */
                        deletePossibleFavorite(parentNodePt->getAddress(), childNodePt->getAddress());

                        /* Delete object */
                        parentNodePt->removeChild(childNodePt);
                        loginChildNodes.removeOne(childNodePt);
                        delete(childNodePt);

                        /* Remove parent */
                        if (deleteEmptyParent)
                        {
                            removeEmptyParentFromDB(parentNodePt, false);
                        }

                        return true;
                    }
                    else
                    {
                        /* Link the chain together */
                        parentNodePt->setStartChildAddress(tempNextChildNodePt->getAddress(), tempNextChildNodePt->getVirtualAddress());
                        tempNextChildNodePt->setPreviousChildAddress(MPNode::EmptyAddress, 0);

                        /* Delete possible fav */
                        deletePossibleFavorite(parentNodePt->getAddress(), childNodePt->getAddress());

                        /* Delete object */
                        parentNodePt->removeChild(childNodePt);
                        loginChildNodes.removeOne(childNodePt);
                        delete(childNodePt);
                        return true;
                    }
                }
                else
                {
                    if (isLastNode)
                    {
                        /* Linked list ends there */
                        tempChildNodePt->setNextChildAddress(MPNode::EmptyAddress, 0);

                        /* Delete possible fav */
                        deletePossibleFavorite(parentNodePt->getAddress(), childNodePt->getAddress());

                        /* Delete object */
                        parentNodePt->removeChild(childNodePt);
                        loginChildNodes.removeOne(childNodePt);
                        delete(childNodePt);
                        return true;
                    }
                    else
                    {
                        /* Link the chain together */
                        tempChildNodePt->setNextChildAddress(tempNextChildNodePt->getAddress(), tempNextChildNodePt->getVirtualAddress());
                        tempNextChildNodePt->setPreviousChildAddress(tempChildNodePt->getAddress(), tempChildNodePt->getVirtualAddress());

                        /* Delete possible fav */
                        deletePossibleFavorite(parentNodePt->getAddress(), childNodePt->getAddress());

                        /* Delete object */
                        parentNodePt->removeChild(childNodePt);
                        loginChildNodes.removeOne(childNodePt);
                        delete(childNodePt);
                        return true;
                    }
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

bool MPDevice::addOrphanChildToDB(MPNode* childNodePt)
{
    QString recovered_service_name = "_recovered_";
    MPNode* tempNodePt;

    qInfo() << "Adding orphan child" << childNodePt->getLogin() << "to DB";

    /* Create a "_recovered_" service */
    tempNodePt = findNodeWithServiceInList(recovered_service_name);
    if (!tempNodePt)
    {
        qInfo() << "No" << recovered_service_name << "service in DB, adding it...";
        tempNodePt = addNewServiceToDB(recovered_service_name);
    }

    /* Add child to DB */
    return addChildToDB(tempNodePt, childNodePt);
}

bool MPDevice::checkLoadedNodes(bool checkCredentials, bool checkData, bool repairAllowed)
{
    QByteArray temp_pnode_address, temp_cnode_address;
    MPNode* temp_pnode_pointer;
    MPNode* temp_cnode_pointer;
    bool return_bool;

    qInfo() << "Checking database...";

    /* Tag pointed nodes, also detects DB errors */
    return_bool = tagPointedNodes(checkCredentials, checkData, repairAllowed);

    /* Scan for orphan nodes */
    quint32 nbOrphanParents = 0;
    quint32 nbOrphanChildren = 0;
    quint32 nbOrphanDataParents = 0;
    quint32 nbOrphanDataChildren = 0;

    if (checkCredentials)
    {
        for (auto &i: loginNodes)
        {
            if (!i->getPointedToCheck())
            {
                qWarning() << "Orphan parent found:" << i->getService() << "at address:" << i->getAddress().toHex();
                if (repairAllowed)
                {
                    addOrphanParentToDB(i, false, repairAllowed);
                }
                nbOrphanParents++;
            }
        }
        for (auto &i: loginChildNodes)
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
                nbOrphanChildren++;
            }
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

    if (checkCredentials)
    {
        /* Last step : check favorites */
        for (auto &i: favoritesAddrs)
        {
            /* Extract parent & child addresses */
            temp_pnode_address = i.mid(0, 2);
            temp_cnode_address = i.mid(2, 2);

            /* Check if favorite is set */
            if ((temp_pnode_address != MPNode::EmptyAddress) || (temp_pnode_address != MPNode::EmptyAddress))
            {
                /* Find the nodes in memory */
                temp_pnode_pointer = findNodeWithAddressInList(loginNodes, temp_pnode_address, 0);
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

void MPDevice::addWriteNodePacketToJob(AsyncJobs *jobs, const QByteArray& address, const QByteArray& data, std::function<void(void)> writeCallback)
{
    for (quint32 i = 0; i < 3; i++)
    {
        quint32 payload_size = MP_MAX_PACKET_LENGTH - MP_PAYLOAD_FIELD_INDEX;
        if (i == 2)
        {
            payload_size = 17;
        }

        QByteArray packetToSend = QByteArray();
        packetToSend.append(address);
        packetToSend.append(i);
        packetToSend.append(data.mid(i*59, payload_size-3));
        //qDebug() << "Write node packet #" << i << " : " << packetToSend.toHex();
        jobs->append(new MPCommandJob(this, MPCmd::WRITE_FLASH_NODE, packetToSend, [=](const QByteArray &data, bool &) -> bool
        {
            if (data[MP_PAYLOAD_FIELD_INDEX] == 0)
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
    }
}

/* Return true if packets need to be sent */
bool MPDevice::generateSavePackets(AsyncJobs *jobs, bool tackleCreds, bool tackleData, MPDeviceProgressCb cbProgress)
{
    qInfo() << "Generating Save Packets...";
    bool diagSavePacketsGenerated = false;
    MPNode* temp_node_pointer;
    progressCurrent = 0;
    progressTotal = 0;

    auto dataWriteProgressCb = [=](void)
    {
        QVariantMap data = {
            {"total", progressTotal},
            {"current", progressCurrent},
            {"msg", "Writing Data To Device: %1/%2 Packets Sent" },
            {"msg_args", QVariantList({progressCurrent, progressTotal})}
        };
        cbProgress(data);
        progressCurrent++;
    };

    /* Change numbers */
    if (isFw12())
    {
        if ((get_credentialsDbChangeNumber() != credentialsDbChangeNumberClone) || (get_dataDbChangeNumber() != dataDbChangeNumberClone))
        {
            diagSavePacketsGenerated = true;
            qDebug() << "Updating cred & data change numbers";
            qDebug() << "Cred DB: " << get_credentialsDbChangeNumber() << " clone: " << credentialsDbChangeNumberClone;
            qDebug() << "Data cred DB: " << get_dataDbChangeNumber() << " clone: " << dataDbChangeNumberClone;
            QByteArray updateChangeNumbersPacket = QByteArray();
            updateChangeNumbersPacket.append(get_credentialsDbChangeNumber());
            updateChangeNumbersPacket.append(get_dataDbChangeNumber());
            jobs->append(new MPCommandJob(this, MPCmd::SET_USER_CHANGE_NB, updateChangeNumbersPacket, MPCommandJob::defaultCheckRet));
        }
    }

    /* First pass: check the nodes that changed or were added */
    if (tackleCreds)
    {
        for (auto &nodelist_iterator: loginNodes)
        {
            /* See if we can find the same node in the clone list */
            temp_node_pointer = findNodeWithAddressInList(loginNodesClone, nodelist_iterator->getAddress(), 0);

            if (!temp_node_pointer)
            {
                qDebug() << "Generating save packet for new service" << nodelist_iterator->getService();
                //qDebug() << "New  contents: " << nodelist_iterator->getNodeData().toHex();
                addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), nodelist_iterator->getNodeData(), dataWriteProgressCb);
                diagSavePacketsGenerated = true;
                progressTotal += 3;
            }
            else if (nodelist_iterator->getNodeData() != temp_node_pointer->getNodeData())
            {
                qDebug() << "Generating save packet for updated service" << nodelist_iterator->getService();
                //qDebug() << "Prev contents: " << temp_node_pointer->getNodeData().toHex();
                //qDebug() << "New  contents: " << nodelist_iterator->getNodeData().toHex();
                addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), nodelist_iterator->getNodeData(),dataWriteProgressCb);
                diagSavePacketsGenerated = true;
                progressTotal += 3;
            }
        }
        for (auto &nodelist_iterator: loginChildNodes)
        {
            /* See if we can find the same node in the clone list */
            temp_node_pointer = findNodeWithAddressInList(loginChildNodesClone, nodelist_iterator->getAddress(), 0);

            if (!temp_node_pointer)
            {
                qDebug() << "Generating save packet for new login" << nodelist_iterator->getLogin();
                //qDebug() << "New  contents: " << nodelist_iterator->getNodeData().toHex();
                addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), nodelist_iterator->getNodeData(), dataWriteProgressCb);
                diagSavePacketsGenerated = true;
                progressTotal += 3;
            }
            else if (nodelist_iterator->getNodeData() != temp_node_pointer->getNodeData())
            {
                qDebug() << "Generating save packet for updated login" << nodelist_iterator->getLogin();
                addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), nodelist_iterator->getNodeData(), dataWriteProgressCb);
                diagSavePacketsGenerated = true;
                progressTotal += 3;
            }
            else
            {
                //qDebug() << "Node data match for login" << nodelist_iterator->getLogin();
            }
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
        for (auto &nodelist_iterator: loginNodesClone)
        {
            /* See if we can find the same node in the clone list */
            temp_node_pointer = findNodeWithAddressInList(loginNodes, nodelist_iterator->getAddress(), 0);

            if (!temp_node_pointer)
            {
                qDebug() << "Generating delete packet for deleted service" << nodelist_iterator->getService();
                addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), QByteArray(MP_NODE_SIZE, 0xFF), dataWriteProgressCb);
                diagSavePacketsGenerated = true;
                progressTotal += 3;
            }
        }
        for (auto &nodelist_iterator: loginChildNodesClone)
        {
            /* See if we can find the same node in the clone list */
            temp_node_pointer = findNodeWithAddressInList(loginChildNodes, nodelist_iterator->getAddress(), 0);

            if (!temp_node_pointer)
            {
                qDebug() << "Generating delete packet for deleted login" << nodelist_iterator->getLogin();
                addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), QByteArray(MP_NODE_SIZE, 0xFF), dataWriteProgressCb);
                diagSavePacketsGenerated = true;
                progressTotal += 3;
            }
        }

        /* Diff favorites */
        for (qint32 i = 0; i < favoritesAddrs.length(); i++)
        {
            if (favoritesAddrs[i] != favoritesAddrsClone[i])
            {
                qDebug() << "Generating favorite" << i << "update packet";
                diagSavePacketsGenerated = true;
                QByteArray updateFavPacket = QByteArray();
                updateFavPacket.append(i);
                updateFavPacket.append(favoritesAddrs[i]);
                jobs->append(new MPCommandJob(this, MPCmd::SET_FAVORITE, updateFavPacket, MPCommandJob::defaultCheckRet));
            }
        }

        /* Diff start node */
        if (startNode != startNodeClone)
        {
            qDebug() << "Updating start node";
            diagSavePacketsGenerated = true;
            jobs->append(new MPCommandJob(this, MPCmd::SET_STARTING_PARENT, startNode, MPCommandJob::defaultCheckRet));
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
                addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), QByteArray(MP_NODE_SIZE, 0xFF), dataWriteProgressCb);
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
                addWriteNodePacketToJob(jobs, nodelist_iterator->getAddress(), QByteArray(MP_NODE_SIZE, 0xFF), dataWriteProgressCb);
                diagSavePacketsGenerated = true;
                progressTotal += 3;
            }
        }

        /* Diff start data node */
        if (startDataNode != startDataNodeClone)
        {
            qDebug() << "Updating start data node";
            diagSavePacketsGenerated = true;
            jobs->append(new MPCommandJob(this, MPCmd::SET_DN_START_PARENT, startDataNode, MPCommandJob::defaultCheckRet));
        }
    }

    /* Diff ctr */
    if (ctrValue != ctrValueClone)
    {
        qDebug() << "Updating CTR value";
        diagSavePacketsGenerated = true;
        jobs->append(new MPCommandJob(this, MPCmd::SET_CTRVALUE, ctrValue, MPCommandJob::defaultCheckRet));
    }

    /* We need to diff cpz ctr values for firmwares running < v1.2 */
    /* Diff: cpzctr values can only be added by design */
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
            jobs->append(new MPCommandJob(this, MPCmd::ADD_CARD_CPZ_CTR, cpzCtrValue[i], MPCommandJob::defaultCheckRet));
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

void MPDevice::exitMemMgmtMode(bool setMMMBool)
{
    AsyncJobs *jobs = new AsyncJobs("Exiting MMM", this);

    jobs->append(new MPCommandJob(this, MPCmd::END_MEMORYMGMT, MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "MMM exit ok";
        cleanMMMVars();
        if (setMMMBool)
        {
            force_memMgmtMode(false);
        }
    });

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
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
                                  [=](const QByteArray &, QByteArray &data_to_send) -> bool
    {
        data_to_send.clear();
        data_to_send.append(Common::dateToBytes(QDate::currentDate()));

        qDebug() << "Sending current date: " <<
                    QString("0x%1").arg((quint8)data_to_send[0], 2, 16, QChar('0')) <<
                    QString("0x%1").arg((quint8)data_to_send[1], 2, 16, QChar('0'));

        return true;
    },
                                [=](const QByteArray &, bool &) -> bool
    {
        return true;
    }));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "Date set success";
    });
    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed to set date on device";
        setCurrentDate(); // memory: does it get piled on?
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
                                [=](const QByteArray &data, bool &) -> bool
    {
        if (data[MP_LEN_FIELD_INDEX] == 1 )
        {
            qWarning() << "Couldn't request uid" << data[MP_PAYLOAD_FIELD_INDEX] <<  data[MP_LEN_FIELD_INDEX] << MPCmd::printCmd(data) << data.toHex();
            set_uid(-1);
            return false;
        }

        bool ok;
        quint64 uid = data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]).toHex().toULongLong(&ok, 16);
        set_uid(ok ? uid : - 1);
        return ok;
    }));

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed get uid from device";
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::getCurrentCardCPZ()
{
    AsyncJobs* cpzjobs = new AsyncJobs("Loading device card CPZ", this);

    /* Query change number */
    cpzjobs->append(new MPCommandJob(this,
                                  MPCmd::GET_CUR_CARD_CPZ,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[MP_PAYLOAD_FIELD_INDEX] == 0)
        {
            qWarning() << "Couldn't request card CPZ";
            return false;
        }
        else
        {
            set_cardCPZ(data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]));
            qDebug() << "Card CPZ: " << get_cardCPZ().toHex();
            if (filesCache.setCardCPZ(get_cardCPZ()))
            {
                qDebug() << "CPZ set to file cache, emitting file cache changed";
                emit filesCacheChanged();
            }
            return true;
        }
    }));

    connect(cpzjobs, &AsyncJobs::finished, [=](const QByteArray &data)
    {
        Q_UNUSED(data);
        //data is last result
        //all jobs finished success
        qInfo() << "Finished loading card CPZ";
    });

    connect(cpzjobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Loading card CPZ failed";
        getCurrentCardCPZ();
    });

    jobsQueue.enqueue(cpzjobs);
    runAndDequeueJobs();
}

void MPDevice::getChangeNumbers()
{
    AsyncJobs* v12jobs = new AsyncJobs("Loading device db change numbers", this);

    /* Query change number */
    v12jobs->append(new MPCommandJob(this,
                                  MPCmd::GET_USER_CHANGE_NB,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[MP_PAYLOAD_FIELD_INDEX] == 0)
        {
            qWarning() << "Couldn't request change numbers";
            return false;
        }
        else
        {
            set_credentialsDbChangeNumber((quint8)data[MP_PAYLOAD_FIELD_INDEX+1]);
            credentialsDbChangeNumberClone = (quint8)data[MP_PAYLOAD_FIELD_INDEX+1];
            set_dataDbChangeNumber((quint8)data[MP_PAYLOAD_FIELD_INDEX+2]);
            dataDbChangeNumberClone = (quint8)data[MP_PAYLOAD_FIELD_INDEX+2];
            if (filesCache.setDbChangeNumber((quint8)data[MP_PAYLOAD_FIELD_INDEX+2]))
            {
                qDebug() << "dbChangeNumber set to file cache, emitting file cache changed";
                emit filesCacheChanged();
            }
            emit dbChangeNumbersChanged(credentialsDbChangeNumberClone, dataDbChangeNumberClone);
            qDebug() << "Credentials change number:" << get_credentialsDbChangeNumber();
            qDebug() << "Data change number:" << get_dataDbChangeNumber();
            return true;
        }
    }));

    connect(v12jobs, &AsyncJobs::finished, [=](const QByteArray &data)
    {
        Q_UNUSED(data);
        //data is last result
        //all jobs finished success
        qInfo() << "Finished loading change numbers";
    });

    connect(v12jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
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

    if (!isFw12())
    {
        qDebug() << "cancelUserRequest not supported for fw < 1.2";
        return;
    }

    qInfo() << "cancel user request (reqid: " << reqid << ")";

    if (currentJobs && currentJobs->getJobsId() == reqid)
    {
        qInfo() << "request_id match current one. Cancel current request";

        QByteArray ba;
        ba.append((char)0);
        ba.append(MPCmd::CANCEL_USER_REQUEST);

        qDebug() << "Platform send command: " << QString("0x%1").arg((quint8)ba[1], 2, 16, QChar('0'));
        platformWrite(ba);
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

    QByteArray sdata = service.toUtf8();
    sdata.append((char)0);

    jobs->append(new MPCommandJob(this, MPCmd::CONTEXT,
                                  sdata,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[MP_PAYLOAD_FIELD_INDEX] != 1)
        {
            if (!fallback_service.isEmpty())
            {
                QByteArray fsdata = fallback_service.toUtf8();
                fsdata.append((char)0);
                jobs->prepend(new MPCommandJob(this, MPCmd::CONTEXT,
                                              fsdata,
                                              [=](const QByteArray &data, bool &) -> bool
                {
                    if (data[MP_PAYLOAD_FIELD_INDEX] != 1)
                    {
                        qWarning() << "Error setting context: " << (quint8)data[2];
                        jobs->setCurrentJobError("failed to select context and fallback_context on device");
                        return false;
                    }

                    QVariantMap m = {{ "service", fallback_service }};
                    jobs->user_data = m;

                    return true;
                }));
                return true;
            }

            qWarning() << "Error setting context: " << (quint8)data[2];
            jobs->setCurrentJobError("failed to select context on device");
            return false;
        }

        QVariantMap m = {{ "service", service }};
        jobs->user_data = m;

        return true;
    }));

    QByteArray ldata = login.toUtf8();
    if (!ldata.isEmpty())
        ldata.append((char)0);

    jobs->append(new MPCommandJob(this, MPCmd::GET_LOGIN,
                                  ldata,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[MP_PAYLOAD_FIELD_INDEX] == 0 && !login.isEmpty())
        {
            jobs->setCurrentJobError("credential access refused by user");
            return false;
        }

        QString l = data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]);
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
                                      [=](const QByteArray &data, bool &) -> bool
        {
            /// Commented below: in case there's an empty description it is impossible to distinguish between device refusal and empty description.
            /*if (data[MP_PAYLOAD_FIELD_INDEX] == 0)
            {
                jobs->setCurrentJobError("failed to query description on device");
                qWarning() << "failed to query description on device";
                return true; //Do not fail if description is not available for this node
            }*/
            QVariantMap m = jobs->user_data.toMap();
            m["description"] = data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]);
            jobs->user_data = m;
            return true;
        }));
    }

    jobs->append(new MPCommandJob(this, MPCmd::GET_PASSWORD,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[MP_PAYLOAD_FIELD_INDEX] == 0)
        {
            jobs->setCurrentJobError("failed to query password on device");
            return false;
        }
        return true;
    }));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &data)
    {
        //data is last result
        //all jobs finished success

        qInfo() << "Password retreived ok";
        QString pass = data.mid(MP_PAYLOAD_FIELD_INDEX, data[MP_LEN_FIELD_INDEX]);

        QVariantMap m = jobs->user_data.toMap();
        cb(true, QString(), m["service"].toString(), m["login"].toString(), pass, m["description"].toString());
    });

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        qCritical() << "Failed getting password: " << failedJob->getErrorStr();
        cb(false, failedJob->getErrorStr(), QString(), QString(), QString(), QString());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::delCredentialAndLeave(QString service, const QString &login,
                                     MPDeviceProgressCb cbProgress,
                                     std::function<void(bool success, QString errstr)> cb)
{
    auto deleteCred = [=]()
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
            setMMCredentials(allCreds, cbProgress, [=](bool success, QString errstr)
            {
                exitMemMgmtMode(); //just in case
                cb(success, errstr);
            });
    };

    if (!get_memMgmtMode())
    {
        startMemMgmtMode(false,
                         cbProgress,
                         [=](bool success, int, QString errMsg)
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

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &data)
    {
        //data is last result
        //all jobs finished success

        qInfo() << "Random numbers generated ok";
        cb(true, QString(), data.mid(0, 32));
    });

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
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
    QByteArray sdata = service.toUtf8();
    sdata.append((char)0);

    quint8 cmdAddCtx = isDataNode?MPCmd::ADD_DATA_SERVICE:MPCmd::ADD_CONTEXT;
    quint8 cmdSelectCtx = isDataNode?MPCmd::SET_DATA_SERVICE:MPCmd::CONTEXT;

    //Create context
    jobs->prepend(new MPCommandJob(this, cmdAddCtx,
                  sdata,
                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[MP_PAYLOAD_FIELD_INDEX] != 1)
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
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[MP_PAYLOAD_FIELD_INDEX] != 1)
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
                             std::function<void(bool success, QString errstr)> cb)
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

    QByteArray sdata = service.toUtf8();
    sdata.append((char)0);

    //First query if context exist
    jobs->append(new MPCommandJob(this, MPCmd::CONTEXT,
                                  sdata,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[MP_PAYLOAD_FIELD_INDEX] != 1)
        {
            qWarning() << "context " << service << " does not exist";
            //Context does not exists, create it
            createJobAddContext(service, jobs);
        }
        else
            qDebug() << "set_context " << service;
        return true;
    }));

    QByteArray ldata = login.toUtf8();
    ldata.append((char)0);

    jobs->append(new MPCommandJob(this, MPCmd::SET_LOGIN,
                                  ldata,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[MP_PAYLOAD_FIELD_INDEX] == 0)
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
        QByteArray ddata = description.toUtf8();
        ddata.append((char)0);

        //Set description should be done right after set login
        jobs->append(new MPCommandJob(this, MPCmd::SET_DESCRIPTION,
                                      ddata,
                                      [=](const QByteArray &data, bool &) -> bool
        {
            if (data[MP_PAYLOAD_FIELD_INDEX] == 0)
            {
                if (description.size() > MOOLTIPASS_DESC_SIZE)
                {
                    qWarning() << "description text is more that " << MOOLTIPASS_DESC_SIZE << " chars";
                    jobs->setCurrentJobError(QString("set_description failed on device, max text length allowed is %1 characters").arg(MOOLTIPASS_DESC_SIZE));
                }
                else
                    jobs->setCurrentJobError("set_description failed on device");
                qWarning() << "Failed to set description to: " << description;
                return false;
            }
            qDebug() << "set_description " << description;
            return true;
        }));
    }

    QByteArray pdata = pass.toUtf8();
    pdata.append((char)0);

    if(!pass.isEmpty()) {
        jobs->append(new MPCommandJob(this, MPCmd::CHECK_PASSWORD,
                                      pdata,
                                      [=](const QByteArray &data, bool &) -> bool
        {
            if (data[MP_PAYLOAD_FIELD_INDEX] != 1)
            {
                //Password does not match, update it
                jobs->prepend(new MPCommandJob(this, MPCmd::SET_PASSWORD,
                                               pdata,
                                               [=](const QByteArray &data, bool &) -> bool
                {
                    if (data[2] == 0)
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

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
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

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        qCritical() << "Failed adding new credential";
        cb(false, failedJob->getErrorStr());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

bool MPDevice::getDataNodeCb(AsyncJobs *jobs,
                             MPDeviceProgressCb cbProgress,
                             const QByteArray &data, bool &)
{
    using namespace std::placeholders;

    //qDebug() << "getDataNodeCb data size: " << ((quint8)data[0]) << "  " << ((quint8)data[1]) << "  " << ((quint8)data[2]);

    if (data[MP_LEN_FIELD_INDEX] == 1 && //data size is 1
        data[MP_PAYLOAD_FIELD_INDEX] == 0)   //value is 0 means end of data
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

    if (data[MP_LEN_FIELD_INDEX] != 0)
    {
        QVariantMap m = jobs->user_data.toMap();
        QByteArray ba = m["data"].toByteArray();

        //first packet, we can read the file size
        if (ba.isEmpty())
        {
            ba.append(data.mid(MP_PAYLOAD_FIELD_INDEX, (int)data.at(0)));
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
            ba.append(data.mid(MP_PAYLOAD_FIELD_INDEX, (int)data.at(0)));
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
        jobs->append(new MPCommandJob(this, MPCmd::READ_32B_IN_DN,
                                      std::bind(&MPDevice::getDataNodeCb, this, jobs, std::move(cbProgress), _1, _2)));
    }
    return true;
}

void MPDevice::getDataNode(QString service, const QString &fallback_service, const QString &reqid,
                           std::function<void(bool success, QString errstr, QString serv, QByteArray rawData)> cb,
                           MPDeviceProgressCb cbProgress)
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

    QByteArray sdata = service.toUtf8();
    sdata.append((char)0);

    jobs->append(new MPCommandJob(this, MPCmd::SET_DATA_SERVICE,
                                  sdata,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[MP_PAYLOAD_FIELD_INDEX] != 1)
        {
            if (!fallback_service.isEmpty())
            {
                QByteArray fsdata = fallback_service.toUtf8();
                fsdata.append((char)0);
                jobs->prepend(new MPCommandJob(this, MPCmd::SET_DATA_SERVICE,
                                              fsdata,
                                              [=](const QByteArray &data, bool &) -> bool
                {
                    if (data[MP_PAYLOAD_FIELD_INDEX] != 1)
                    {
                        qWarning() << "Error setting context: " << (quint8)data[2];
                        jobs->setCurrentJobError("failed to select context and fallback_context on device");
                        return false;
                    }

                    QVariantMap m = {{ "service", fallback_service }};
                    jobs->user_data = m;

                    return true;
                }));
                return true;
            }

            qWarning() << "Error setting context: " << (quint8)data[2];
            jobs->setCurrentJobError("failed to select context on device");
            return false;
        }

        QVariantMap m = {{ "service", service }};
        jobs->user_data = m;

        return true;
    }));

    using namespace std::placeholders;

    //ask for the first 32bytes packet
    //bind to a member function of MPDevice, to be able to loop over until with got all the data
    jobs->append(new MPCommandJob(this, MPCmd::READ_32B_IN_DN,
                                  std::bind(&MPDevice::getDataNodeCb, this, jobs, std::move(cbProgress), _1, _2)));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        //all jobs finished success
        qInfo() << "get_data_node success";
        QVariantMap m = jobs->user_data.toMap();
        QByteArray ndata = m["data"].toByteArray();

        //check data size
        quint32 sz = qFromBigEndian<quint32>((quint8 *)ndata.data());
        qDebug() << "Data size: " << sz;

        cb(true, QString(), m["service"].toString(), ndata.mid(4, sz));
    });

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        qCritical() << "Failed getting data node";
        cb(false, failedJob->getErrorStr(), QString(), QByteArray());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

bool MPDevice::setDataNodeCb(AsyncJobs *jobs, int current,
                             MPDeviceProgressCb cbProgress,
                             const QByteArray &data, bool &)
{
    using namespace std::placeholders;

    qDebug() << "setDataNodeCb data current: " << current;

    if (data[MP_PAYLOAD_FIELD_INDEX] == 0)
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
    jobs->append(new MPCommandJob(this, MPCmd::WRITE_32B_IN_DN,
                                  packet,
                                  std::bind(&MPDevice::setDataNodeCb, this, jobs, current + MOOLTIPASS_BLOCK_SIZE,
                                            std::move(cbProgress), _1, _2)));

    return true;
}

void MPDevice::setDataNode(QString service, const QByteArray &nodeData,
                           std::function<void(bool success, QString errstr)> cb,
                           MPDeviceProgressCb cbProgress)
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

    QByteArray sdata = service.toUtf8();
    sdata.append((char)0);

    jobs->append(new MPCommandJob(this, MPCmd::SET_DATA_SERVICE,
                                  sdata,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[MP_PAYLOAD_FIELD_INDEX] != 1)
        {
            qWarning() << "context " << service << " does not exist";
            //Context does not exists, create it
            createJobAddContext(service, jobs, true);
        }
        else
            qDebug() << "set_data_context " << service;
        return true;
    }));

    using namespace std::placeholders;

    //set size of data
    currentDataNode = QByteArray();
    currentDataNode.resize(MP_DATA_HEADER_SIZE);
    qToBigEndian(nodeData.size(), (quint8 *)currentDataNode.data());
    currentDataNode.append(nodeData);

    //first packet
    QByteArray firstPacket;
    char eod = (nodeData.size() + MP_DATA_HEADER_SIZE <= MOOLTIPASS_BLOCK_SIZE)?1:0;
    firstPacket.append(eod);
    firstPacket.append(currentDataNode.mid(0, MOOLTIPASS_BLOCK_SIZE));
    firstPacket.resize(MOOLTIPASS_BLOCK_SIZE + 1);

//    cbProgress(currentDataNode.size() - MP_DATA_HEADER_SIZE, MOOLTIPASS_BLOCK_SIZE);

    //send the first 32bytes packet
    //bind to a member function of MPDevice, to be able to loop over until with got all the data
    jobs->append(new MPCommandJob(this, MPCmd::WRITE_32B_IN_DN,
                                  firstPacket,
                                  std::bind(&MPDevice::setDataNodeCb, this, jobs, MOOLTIPASS_BLOCK_SIZE,
                                            std::move(cbProgress), _1, _2)));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
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

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        qCritical() << "Failed writing data node";
        cb(false, failedJob->getErrorStr());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void  MPDevice::deleteDataNodesAndLeave(QStringList services,
                                        std::function<void(bool success, QString errstr)> cb,
                                        MPDeviceProgressCb cbProgress)
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

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &data)
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
        connect(saveJobs, &AsyncJobs::finished, [=](const QByteArray &data)
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
        connect(saveJobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
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
            if ((services.size() > 0) && isFw12())
            {
                set_dataDbChangeNumber(get_dataDbChangeNumber() + 1);
                dataDbChangeNumberClone = get_dataDbChangeNumber();
                filesCache.setDbChangeNumber(get_dataDbChangeNumber());
                QByteArray updateChangeNumbersPacket = QByteArray();
                updateChangeNumbersPacket.append(get_credentialsDbChangeNumber());
                updateChangeNumbersPacket.append(get_dataDbChangeNumber());
                saveJobs->append(new MPCommandJob(this, MPCmd::SET_USER_CHANGE_NB, updateChangeNumbersPacket, MPCommandJob::defaultCheckRet));
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

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Couldn't Rescan The Memory";
        exitMemMgmtMode(true);
        cb(false, "Couldn't Rescan The Memory");
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::changeVirtualAddressesToFreeAddresses(void)
{
    if (virtualStartNode != 0)
    {
        qDebug() << "Setting start node to " << freeAddresses[virtualStartNode].toHex();
        startNode = freeAddresses[virtualStartNode];
    }
    if (virtualDataStartNode != 0)
    {
        qDebug() << "Setting data start node to " << freeAddresses[virtualDataStartNode].toHex();
        startDataNode = freeAddresses[virtualDataStartNode];
    }
    qDebug() << "Replacing virtual addresses for login nodes...";
    for (auto &i: loginNodes)
    {
        if (i->getAddress().isNull()) i->setAddress(freeAddresses[i->getVirtualAddress()]);
        if (i->getNextParentAddress().isNull()) i->setNextParentAddress(freeAddresses[i->getNextParentVirtualAddress()]);
        if (i->getPreviousParentAddress().isNull()) i->setPreviousParentAddress(freeAddresses[i->getPreviousParentVirtualAddress()]);
        if (i->getStartChildAddress().isNull()) i->setStartChildAddress(freeAddresses[i->getStartChildVirtualAddress()]);
    }
    qDebug() << "Replacing virtual addresses for child nodes...";
    for (auto &i: loginChildNodes)
    {
        if (i->getAddress().isNull()) i->setAddress(freeAddresses[i->getVirtualAddress()]);
        if (i->getNextChildAddress().isNull()) i->setNextChildAddress(freeAddresses[i->getNextChildVirtualAddress()]);
        if (i->getPreviousChildAddress().isNull()) i->setPreviousChildAddress(freeAddresses[i->getPreviousChildVirtualAddress()]);
    }
    qDebug() << "Replacing virtual addresses for data nodes...";
    for (auto &i: dataNodes)
    {
        if (i->getAddress().isNull()) i->setAddress(freeAddresses[i->getVirtualAddress()]);
        if (i->getNextParentAddress().isNull()) i->setNextParentAddress(freeAddresses[i->getNextParentVirtualAddress()]);
        if (i->getPreviousParentAddress().isNull()) i->setPreviousParentAddress(freeAddresses[i->getPreviousParentVirtualAddress()]);
        if (i->getStartChildAddress().isNull()) i->setStartChildAddress(freeAddresses[i->getStartChildVirtualAddress()]);
    }
    qDebug() << "Replacing virtual addresses for data child nodes...";
    for (auto &i: dataChildNodes)
    {
        if (i->getAddress().isNull()) i->setAddress(freeAddresses[i->getVirtualAddress()]);
        if (i->getNextChildDataAddress().isNull()) i->setNextChildDataAddress(freeAddresses[i->getNextChildVirtualAddress()]);
    }
}

quint64 MPDevice::getUInt64EncryptionKey()
{
    qint64 key = 0;
    for (int i = 0; i < std::min(8, m_cardCPZ.size()) ; i ++)
        key += (static_cast<unsigned int>(m_cardCPZ[i]) & 0xFF) << (i*8);

    return key;
}

QString MPDevice::encryptSimpleCrypt(const QByteArray &data)
{
    /* Encrypt payload */
    SimpleCrypt simpleCrypt;
    simpleCrypt.setKey(getUInt64EncryptionKey());

    return simpleCrypt.encryptToString(data);
}

QByteArray MPDevice::decryptSimpleCrypt(const QString &payload)
{
    SimpleCrypt simpleCrypt;
    simpleCrypt.setKey(getUInt64EncryptionKey());

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
    startNode = loginNodes[1]->getAddress();
    loginNodes[1]->setPreviousParentAddress(MPNode::EmptyAddress);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Skipping first parent node: test failed!";return false;} else qInfo() << "Skipping first parent node: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Skipping last parent node";
    loginNodes[loginNodes.size()-2]->setNextParentAddress(MPNode::EmptyAddress);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Skipping last parent node: test failed!";return false;} else qInfo() << "Skipping last parent node: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Setting invalid startNode";
    startNode = invalidAddress;
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
    temp_node = new MPNode(temp_node_pt->getNodeData(), this, temp_node_pt->getAddress(), temp_node_pt->getVirtualAddress());
    removeChildFromDB(loginNodes[0], temp_node_pt, true);
    addChildToDB(loginNodes[0], temp_node);
    loginChildNodes.append(temp_node);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Removing & Adding First Child Node: test failed!";return false;} else qInfo() << "Removing & Adding First Child Node: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Deleting middle child node and adding it again";
    temp_node_pt = findNodeWithAddressInList(loginChildNodes, loginNodes[0]->getStartChildAddress(), loginNodes[0]->getStartChildVirtualAddress());
    temp_node_pt = findNodeWithAddressInList(loginChildNodes, temp_node_pt->getNextChildAddress(), temp_node_pt->getNextChildVirtualAddress());
    temp_node = new MPNode(temp_node_pt->getNodeData(), this, temp_node_pt->getAddress(), temp_node_pt->getVirtualAddress());
    removeChildFromDB(loginNodes[0], temp_node_pt, true);
    addChildToDB(loginNodes[0], temp_node);
    loginChildNodes.append(temp_node);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Removing & Adding Middle Child Node: test failed!";return false;} else qInfo() << "Removing & Adding Middle Child Node: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Deleting last child node and adding it again";
    temp_node_pt = findNodeWithAddressInList(loginChildNodes, loginNodes[0]->getStartChildAddress(), loginNodes[0]->getStartChildVirtualAddress());
    temp_node_pt = findNodeWithAddressInList(loginChildNodes, temp_node_pt->getNextChildAddress(), temp_node_pt->getNextChildVirtualAddress());
    temp_node_pt = findNodeWithAddressInList(loginChildNodes, temp_node_pt->getNextChildAddress(), temp_node_pt->getNextChildVirtualAddress());
    temp_node = new MPNode(temp_node_pt->getNodeData(), this, temp_node_pt->getAddress(), temp_node_pt->getVirtualAddress());
    removeChildFromDB(loginNodes[0], temp_node_pt, true);
    addChildToDB(loginNodes[0], temp_node);
    loginChildNodes.append(temp_node);
    checkLoadedNodes(true, true, true);
    if (generateSavePackets(jobs, true, true, ignoreProgressCb)) {qCritical() << "Removing & Adding Last Child Node: test failed!";return false;} else qInfo() << "Removing & Adding Last Child Node: passed!";

    qInfo() << "testCodeAgainstCleanDBChanges: Deleting parent node with single child node and adding it again";
    temp_node_pt = findNodeWithAddressInList(loginChildNodes, loginNodes[1]->getStartChildAddress(), loginNodes[1]->getStartChildVirtualAddress());
    temp_cnode = new MPNode(temp_node_pt->getNodeData(), this, temp_node_pt->getAddress(), temp_node_pt->getVirtualAddress());
    temp_node = new MPNode(loginNodes[1]->getNodeData(), this, loginNodes[1]->getAddress(), loginNodes[1]->getVirtualAddress());
    temp_node->setStartChildAddress(MPNode::EmptyAddress, 0);
    removeChildFromDB(loginNodes[1], temp_node_pt, true);
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
    exportTopArray.append(QJsonValue(Common::bytesToJson(startNode)));

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

    /* Generate file payload */
    QJsonDocument payloadDoc(exportTopArray);
    auto payload = payloadDoc.toJson();

    if (encryption.isEmpty() || encryption == "none")
        return payload;

    /* Export file content */
    QJsonObject exportTopObject;

    if (encryption == "SimpleCrypt") {
        exportTopObject.insert("encryption", "SimpleCrypt");
        exportTopObject.insert("payload", encryptSimpleCrypt(payload));
    } else
    {
        // Fallback in case of an unknown encryption method where specified
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
            if ( encryptionMethod == "SimpleCrypt")
            {
                QString payload = importFile.value("payload").toString();
                auto decryptedData = decryptSimpleCrypt(payload);

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

bool MPDevice::readExportPayload(QJsonArray dataArray, QString &errorString)
{
    /** Mooltiapp / Chrome App save file **/

    /* Checks */
    if (!((dataArray[9].toString() == "mooltipass" && dataArray.size() == 10) || (dataArray[9].toString() == "moolticute" && dataArray.size() == 14)))
    {
        qCritical() << "Invalid MooltiApp file";
        errorString = "Selected File Isn't Correct";
        return false;
    }

    /* Know which bundle we're dealing with */
    if (dataArray[9].toString() == "mooltipass")
    {
        isMooltiAppImportFile = true;
        qInfo() << "Dealing with MooltiApp export file";
    }
    else
    {
        qInfo() << "Dealing with Moolticute export file";
        isMooltiAppImportFile = false;
        importedCredentialsDbChangeNumber = dataArray[11].toInt();
        qDebug() << "Imported cred change number: " << importedCredentialsDbChangeNumber;
        importedDataDbChangeNumber = dataArray[12].toInt();
        qDebug() << "Imported data change number: " << importedDataDbChangeNumber;
        importedDbMiniSerialNumber = dataArray[13].toInt();
        qDebug() << "Imported mini serial number: " << importedDbMiniSerialNumber;
    }

    /* Read CTR */
    importedCtrValue = QByteArray();
    auto qjobject = dataArray[0].toObject();
    for (qint32 i = 0; i < qjobject.size(); i++) {importedCtrValue.append(qjobject[QString::number(i)].toInt());}
    qDebug() << "Imported CTR: " << importedCtrValue.toHex();

    /* Read CPZ CTR values */
    auto qjarray = dataArray[1].toArray();
    for (qint32 i = 0; i < qjarray.size(); i++)
    {
        qjobject = qjarray[i].toObject();
        QByteArray qbarray = QByteArray();
        for (qint32 j = 0; j < qjobject.size(); j++) {qbarray.append(qjobject[QString::number(j)].toInt());}
        qDebug() << "Imported CPZ/CTR value : " << qbarray.toHex();
        importedCpzCtrValue.append(qbarray);
    }

    /* Check if one of them is for the current card */
    bool cpzFound = false;
    for (qint32 i = 0; i < importedCpzCtrValue.size(); i++)
    {
        if (importedCpzCtrValue[i].mid(0, 8) == get_cardCPZ())
        {
            qDebug() << "Import file is a backup for current user";
            unknownCardAddPayload = importedCpzCtrValue[i];
            cpzFound = true;
        }
    }
    if (!cpzFound)
    {
        qWarning() << "Import file is not a backup for current user";
        errorString = "Selected File Is Another User's Backup";
        return false;
    }

    /* Read Starting Parent */
    importedStartNode = QByteArray();
    qjarray = dataArray[2].toArray();
    for (qint32 i = 0; i < qjarray.size(); i++) {importedStartNode.append(qjarray[i].toInt());}
    qDebug() << "Imported start node: " << importedStartNode.toHex();

    /* Read Data Starting Parent */
    importedStartDataNode = QByteArray();
    qjarray = dataArray[3].toArray();
    for (qint32 i = 0; i < qjarray.size(); i++) {importedStartDataNode.append(qjarray[i].toInt());}
    qDebug() << "Imported data start node: " << importedStartDataNode.toHex();

    /* Read favorites */
    qjarray = dataArray[4].toArray();
    for (qint32 i = 0; i < qjarray.size(); i++)
    {
        qjobject = qjarray[i].toObject();
        QByteArray qbarray = QByteArray();
        for (qint32 j = 0; j < qjobject.size(); j++) {qbarray.append(qjobject[QString::number(j)].toInt());}
        qDebug() << "Imported favorite " << i << " : " << qbarray.toHex();
        importedFavoritesAddrs.append(qbarray);
    }

    /* Read service nodes */
    qjarray = dataArray[5].toArray();
    for (qint32 i = 0; i < qjarray.size(); i++)
    {
        qjobject = qjarray[i].toObject();

        /* Fetch address */
        QJsonArray serviceAddrArr = qjobject["address"].toArray();
        QByteArray serviceAddr = QByteArray();
        for (qint32 j = 0; j < serviceAddrArr.size(); j++) {serviceAddr.append(serviceAddrArr[j].toInt());}

        /* Fetch core data */
        QJsonObject dataObj = qjobject["data"].toObject();
        QByteArray dataCore = QByteArray();
        for (qint32 j = 0; j < dataObj.size(); j++) {dataCore.append(dataObj[QString::number(j)].toInt());}

        /* Recreate node and add it to the list of imported nodes */
        MPNode* importedNode = new MPNode(dataCore, this, serviceAddr, 0);
        importedLoginNodes.append(importedNode);
        //qDebug() << "Parent nodes: imported " << qjobject["name"].toString();
    }

    /* Read service child nodes */
    qjarray = dataArray[6].toArray();
    for (qint32 i = 0; i < qjarray.size(); i++)
    {
        qjobject = qjarray[i].toObject();

        /* Fetch address */
        QJsonArray serviceAddrArr = qjobject["address"].toArray();
        QByteArray serviceAddr = QByteArray();
        for (qint32 j = 0; j < serviceAddrArr.size(); j++) {serviceAddr.append(serviceAddrArr[j].toInt());}

        /* Fetch core data */
        QJsonObject dataObj = qjobject["data"].toObject();
        QByteArray dataCore = QByteArray();
        for (qint32 j = 0; j < dataObj.size(); j++) {dataCore.append(dataObj[QString::number(j)].toInt());}

        /* Recreate node and add it to the list of imported nodes */
        MPNode* importedNode = new MPNode(dataCore, this, serviceAddr, 0);
        importedLoginChildNodes.append(importedNode);
        //qDebug() << "Child nodes: imported " << qjobject["name"].toString();
    }

    if (!isMooltiAppImportFile)
    {
        /* Read service nodes */
        qjarray = dataArray[7].toArray();
        for (qint32 i = 0; i < qjarray.size(); i++)
        {
            qjobject = qjarray[i].toObject();

            /* Fetch address */
            QJsonArray serviceAddrArr = qjobject["address"].toArray();
            QByteArray serviceAddr = QByteArray();
            for (qint32 j = 0; j < serviceAddrArr.size(); j++) {serviceAddr.append(serviceAddrArr[j].toInt());}

            /* Fetch core data */
            QJsonObject dataObj = qjobject["data"].toObject();
            QByteArray dataCore = QByteArray();
            for (qint32 j = 0; j < dataObj.size(); j++) {dataCore.append(dataObj[QString::number(j)].toInt());}

            /* Recreate node and add it to the list of imported nodes */
            MPNode* importedNode = new MPNode(dataCore, this, serviceAddr, 0);
            importedDataNodes.append(importedNode);
            //qDebug() << "Parent nodes: imported " << qjobject["name"].toString();
        }

        /* Read service child nodes */
        qjarray = dataArray[8].toArray();
        for (qint32 i = 0; i < qjarray.size(); i++)
        {
            qjobject = qjarray[i].toObject();

            /* Fetch address */
            QJsonArray serviceAddrArr = qjobject["address"].toArray();
            QByteArray serviceAddr = QByteArray();
            for (qint32 j = 0; j < serviceAddrArr.size(); j++) {serviceAddr.append(serviceAddrArr[j].toInt());}

            /* Fetch core data */
            QJsonObject dataObj = qjobject["data"].toObject();
            QByteArray dataCore = QByteArray();
            for (qint32 j = 0; j < dataObj.size(); j++) {dataCore.append(dataObj[QString::number(j)].toInt());}

            /* Recreate node and add it to the list of imported nodes */
            MPNode* importedNode = new MPNode(dataCore, this, serviceAddr, 0);
            importedDataChildNodes.append(importedNode);
            //qDebug() << "Child nodes: imported " << qjobject["name"].toString();
        }
    }

    return true;
}

void MPDevice::cleanImportedVars(void)
{
    virtualStartNode = 0;
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
    importedLoginNodes.clear();
    importedLoginChildNodes.clear();
    importedDataNodes.clear();
    importedDataChildNodes.clear();
}

void MPDevice::cleanMMMVars(void)
{
    /* Cleaning all temp values */
    virtualStartNode = 0;
    virtualDataStartNode = 0;
    ctrValue.clear();
    cpzCtrValue.clear();
    qDeleteAll(loginNodes);
    loginNodes.clear();
    qDeleteAll(dataNodes);
    dataNodes.clear();
    favoritesAddrs.clear();
    loginChildNodes.clear();
    dataChildNodes.clear();
    /* Cleaning the clones as well */
    ctrValueClone.clear();
    cpzCtrValueClone.clear();
    qDeleteAll(loginNodesClone);
    loginNodesClone.clear();
    qDeleteAll(dataNodesClone);
    dataNodesClone.clear();
    favoritesAddrsClone.clear();
    loginChildNodesClone.clear();
    dataChildNodesClone.clear();
    freeAddresses.clear();
}

void MPDevice::startImportFileMerging(MPDeviceProgressCb cbProgress, std::function<void(bool success, QString errstr)> cb, bool noDelete)
{
    /* New job for starting MMM */
    AsyncJobs *jobs = new AsyncJobs("Starting MMM mode for import file merging", this);

    /* Ask device to go into MMM first */
    jobs->append(new MPCommandJob(this, MPCmd::START_MEMORYMGMT, MPCommandJob::defaultCheckRet));

    /* Load flash contents the usual way */
    memMgmtModeReadFlash(jobs, false,
                            cbProgress,
                            true, true, true);

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &data)
    {
        Q_UNUSED(data);
        qInfo() << "Mem management mode enabled";

        /* Tag favorites */
        tagFavoriteNodes();

        /* We arrive here knowing that the CPZ is in the CPZ/CTR list */
        qInfo() << "Starting File Merging...";

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

        /// Check CTR value
        quint32 cur_ctr = ((quint8)ctrValue[0])*256*256 + ((quint8)ctrValue[1])*256 + ((quint8)ctrValue[2]);
        quint32 imported_ctr = ((quint8)importedCtrValue[0])*256*256 + ((quint8)importedCtrValue[1]*256) + ((quint8)importedCtrValue[2]);
        if (imported_ctr > cur_ctr)
        {
            qDebug() << "CTR value mismatch: " << ctrValue.toHex() << " instead of " << importedCtrValue.toHex();
            ctrValue = QByteArray(importedCtrValue);
        }

        /// Find the nodes we don't have in memory or that have been changed
        for (qint32 i = 0; i < importedLoginNodes.size(); i++)
        {
            bool service_node_found = false;

            // Loop in the memory nodes to compare data
            for (qint32 j = 0; j < loginNodes.size(); j++)
            {
                if (importedLoginNodes[i]->getLoginNodeData() == loginNodes[j]->getLoginNodeData())
                {
                    // We found a parent node that has the same core data (doesn't mean the same prev / next node though!)
                    //qDebug() << "Parent node core data match for " << importedLoginNodes[i]->getService();
                    loginNodes[j]->setMergeTagged();
                    service_node_found = true;

                    // Next step is to check if the children are the same
                    quint32 cur_import_child_node_addr_v = importedLoginNodes[i]->getStartChildVirtualAddress();
                    QByteArray cur_import_child_node_addr = importedLoginNodes[i]->getStartChildAddress();
                    quint32 matched_parent_first_child_v = loginNodes[j]->getStartChildVirtualAddress();
                    QByteArray matched_parent_first_child = loginNodes[j]->getStartChildAddress();

                    /* Special case: parent doesn't have children but we do */
                    if (((cur_import_child_node_addr == MPNode::EmptyAddress) || (cur_import_child_node_addr.isNull() && cur_import_child_node_addr_v == 0)) && ((matched_parent_first_child != MPNode::EmptyAddress) || (matched_parent_first_child.isNull() && matched_parent_first_child_v != 0)))
                    {
                        loginNodes[j]->setStartChildAddress(MPNode::EmptyAddress);
                    }

                    //qDebug() << "First child address for imported node: " << cur_import_child_node_addr.toHex() << " , for own node: " << matched_parent_first_child.toHex();
                    while ((cur_import_child_node_addr != MPNode::EmptyAddress) || (cur_import_child_node_addr.isNull() && cur_import_child_node_addr_v != 0))
                    {
                        // Find the imported child node in our list
                        MPNode* imported_child_node = findNodeWithAddressInList(importedLoginChildNodes, cur_import_child_node_addr, cur_import_child_node_addr_v);

                        // Check if we actually found the node
                        if (!imported_child_node)
                        {
                            cleanImportedVars();
                            exitMemMgmtMode(false);
                            cb(false, "Couldn't Import Database: Corrupted Export File");
                            qCritical() << "Couldn't find imported child node in our list (corrupted import file?)";
                            return;
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
                            MPNode* cur_child_node = findNodeWithAddressInList(loginChildNodes, matched_parent_next_child, matched_parent_next_child_v);

                            if (!cur_child_node)
                            {
                                cleanImportedVars();
                                exitMemMgmtMode(false);
                                cb(false, "Couldn't Import Database: Please Run Integrity Check");
                                qCritical() << "Couldn't find child node in our list (bad node reading?)";
                                return;
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
                                    //qDebug() << importedLoginNodes[i]->getService() << " : child core data match for child " << imported_child_node->getLogin() << " , nothing to do";
                                }
                                else
                                {
                                    // Data mismatch, overwrite the important part
                                    qDebug() << importedLoginNodes[i]->getService() << " : child core data mismatch for child " << imported_child_node->getLogin() << " , updating...";
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
                            qDebug() << importedLoginNodes[i]->getService() << " : adding new child " << imported_child_node->getLogin() << " in the mooltipass...";

                            /* Increment new addresses counter */
                            newAddressesNeededCounter += 1;

                            /* Create new node with null address and virtual address set to our counter value */
                            MPNode* newChildNodePt = new MPNode(QByteArray(MP_NODE_SIZE, 0), this, QByteArray(), newAddressesNeededCounter);
                            newChildNodePt->setLoginChildNodeData(imported_child_node->getNodeFlags(), imported_child_node->getLoginChildNodeData());
                            newChildNodePt->setMergeTagged();

                            /* Add node to list */
                            loginChildNodes.append(newChildNodePt);
                            if (!addChildToDB(loginNodes[j], newChildNodePt))
                            {
                                cleanImportedVars();
                                exitMemMgmtMode(false);
                                cb(false, "Couldn't Import Database: Please Run Integrity Check");
                                qCritical() << "Couldn't add new child node to DB (corrupted DB?)";
                                return;
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
               newAddressesNeededCounter += 1;

               /* Create new node with null address and virtual address set to our counter value */
               MPNode* newNodePt = new MPNode(QByteArray(MP_NODE_SIZE, 0), this, QByteArray(), newAddressesNeededCounter);
               newNodePt->setLoginNodeData(importedLoginNodes[i]->getNodeFlags(), importedLoginNodes[i]->getLoginNodeData());
               newNodePt->setMergeTagged();

               /* Add node to list */
               loginNodes.append(newNodePt);
               if (!addOrphanParentToDB(newNodePt, false, false))
               {
                   cleanImportedVars();
                   exitMemMgmtMode(false);
                   qCritical() << "Couldn't add parent to DB (corrupted DB?)";
                   cb(false, "Couldn't Import Database: Please Run Integrity Check");
                   return;
               }

               /* Next step is to follow the children */
               QByteArray curImportChildAddr = importedLoginNodes[i]->getStartChildAddress();
               while (curImportChildAddr != MPNode::EmptyAddress)
               {
                   /* Find node in list */
                   MPNode* curImportChildPt = findNodeWithAddressInList(importedLoginChildNodes, curImportChildAddr);

                   if (!curImportChildPt)
                   {
                       cleanImportedVars();
                       exitMemMgmtMode(false);
                       cb(false, "Couldn't Import Database: Corrupted Import File");
                       qCritical() << "Couldn't find import child (import file problem?)";
                       return;
                   }

                   /* Increment new addresses counter */
                   newAddressesNeededCounter += 1;

                   /* Create new node with null address and virtual address set to our counter value */
                   MPNode* newChildNodePt = new MPNode(QByteArray(MP_NODE_SIZE, 0), this, QByteArray(), newAddressesNeededCounter);
                   newChildNodePt->setLoginChildNodeData(curImportChildPt->getNodeFlags(), curImportChildPt->getLoginChildNodeData());
                   newChildNodePt->setMergeTagged();

                   /* Add node to list */
                   loginChildNodes.append(newChildNodePt);
                   if (!addChildToDB(newNodePt, newChildNodePt))
                   {
                       cleanImportedVars();
                       exitMemMgmtMode(false);
                       cb(false, "Couldn't Import Database: Please Run Integrity Check");
                       qCritical() << "Couldn't add new child node to DB (corrupted DB?)";
                       return;
                   }

                   /* Go to the next child */
                   curImportChildAddr = curImportChildPt->getNextChildAddress();
               }
            }
        }

        /// Same but for data nodes
        if (!isMooltiAppImportFile)
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
                                return;
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
                                        return;
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
                                                return;
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
                                newAddressesNeededCounter += 1;

                                /* Create new node with null address and virtual address set to our counter value */
                                MPNode* newDataChildNodePt = new MPNode(QByteArray(MP_NODE_SIZE, 0), this, QByteArray(), newAddressesNeededCounter);
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
                   newAddressesNeededCounter += 1;

                   /* Create new node with null address and virtual address set to our counter value */
                   MPNode* newNodePt = new MPNode(QByteArray(MP_NODE_SIZE, 0), this, QByteArray(), newAddressesNeededCounter);
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
                       return;
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
                           return;
                       }

                       /* Increment new addresses counter */
                       newAddressesNeededCounter += 1;

                       /* Create new node with null address and virtual address set to our counter value */
                       MPNode* newDataChildNodePt = new MPNode(QByteArray(MP_NODE_SIZE, 0), this, QByteArray(), newAddressesNeededCounter);
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
        }

        qInfo() << newAddressesNeededCounter << " addresses are required for merge operations ";

        /* If we need addresses, query them */
        if (newAddressesNeededCounter > 0)
        {
            /* Add one extra address because we do pre-increment on that counter */
            newAddressesNeededCounter++;

            AsyncJobs* getFreeAddressesJob = new AsyncJobs("Asking free adresses...", this);
            loadFreeAddresses(getFreeAddressesJob, MPNode::EmptyAddress, false, cbProgress);

            connect(getFreeAddressesJob, &AsyncJobs::finished, [=](const QByteArray &data)
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
                    connect(mergeOperations, &AsyncJobs::finished, [=](const QByteArray &data)
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
                    connect(mergeOperations, &AsyncJobs::failed, [=](AsyncJob *failedJob)
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

            connect(getFreeAddressesJob, &AsyncJobs::failed, [=](AsyncJob *failedJob)
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
                connect(mergeOperations, &AsyncJobs::finished, [=](const QByteArray &data)
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
                connect(mergeOperations, &AsyncJobs::failed, [=](AsyncJob *failedJob)
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

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Setting device in MMM failed";
        cb(false, "Please Retry and Approve Credential Management");
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

bool MPDevice::finishImportFileMerging(QString &stringError, bool noDelete)
{
    qInfo() << "Finishing Import File Merging...";

    if (!noDelete)
    {
        /* Now we check all our parents and childs for non merge tag */
        QListIterator<MPNode*> i(loginNodes);
        while (i.hasNext())
        {
            MPNode* nodeItem = i.next();

            /* No need to check for merge tagged for parent, as it'll automatically be removed if it doesn't have any child */
            QByteArray curChildNodeAddr = nodeItem->getStartChildAddress();

            /* Special case: no child */
            if (curChildNodeAddr == MPNode::EmptyAddress)
            {
                /* Remove parent */
                removeEmptyParentFromDB(nodeItem, false);
            }

            /* Check every children */
            while (curChildNodeAddr != MPNode::EmptyAddress)
            {
                MPNode* curNode = findNodeWithAddressInList(loginChildNodes, curChildNodeAddr);

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
                    removeChildFromDB(nodeItem, curNode, true);
                }
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

void MPDevice::loadFreeAddresses(AsyncJobs *jobs, const QByteArray &addressFrom, bool discardFirstAddr, MPDeviceProgressCb cbProgress)
{
    qDebug() << "Loading free addresses from address:" << addressFrom.toHex();

    jobs->append(new MPCommandJob(this, MPCmd::GET_30_FREE_SLOTS,
                                  addressFrom,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        quint32 nb_free_addresses_received = data[MP_LEN_FIELD_INDEX]/2;
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
                    freeAddresses.append(data.mid(MP_PAYLOAD_FIELD_INDEX + 2 + i*2, 2));
                    //qDebug() << "Received free address " << data.mid(MP_PAYLOAD_FIELD_INDEX + 2 + i*2, 2).toHex();
                }
                else
                {
                    freeAddresses.append(data.mid(MP_PAYLOAD_FIELD_INDEX + i*2, 2));
                    //qDebug() << "Received free address " << data.mid(MP_PAYLOAD_FIELD_INDEX + i*2, 2).toHex();
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

void MPDevice::startIntegrityCheck(std::function<void(bool success, QString errstr)> cb,
                                   MPDeviceProgressCb cbProgress)
{
    /* New job for starting MMM */
    AsyncJobs *jobs = new AsyncJobs("Starting integrity check", this);

    /* Ask device to go into MMM first */
    jobs->append(new MPCommandJob(this, MPCmd::START_MEMORYMGMT, MPCommandJob::defaultCheckRet));

    /* Ask one free address just in case we need it for creating a _recovered_ service */
    newAddressesNeededCounter = 1;
    newAddressesReceivedCounter = 0;
    loadFreeAddresses(jobs, MPNode::EmptyAddress, false, cbProgress);

    /* Setup global vars dedicated to speed diagnostics */
    diagNbBytesRec = 0;
    diagLastNbBytesPSec = 0;
    lastFlashPageScanned = 0;
    diagLastSecs = QDateTime::currentMSecsSinceEpoch()/1000;

    /* Load CTR, favorites, nodes... */
    memMgmtModeReadFlash(jobs, true, cbProgress, true, true, true);

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "Finished loading the nodes in memory";

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
        repairJobs->append(new MPCommandJob(this, MPCmd::END_MEMORYMGMT, MPCommandJob::defaultCheckRet));

        connect(repairJobs, &AsyncJobs::finished, [=](const QByteArray &data)
        {
            Q_UNUSED(data);

            if (packets_generated)
            {
                qInfo() << "Found and Corrected Errors in Database";
                cb(true, "Errors Were Found And Corrected In The Database");
            }
            else
            {
                qInfo() << "Nothing to correct in DB";
                cb(true, "Database Is Free Of Errors");
            }
        });

        connect(repairJobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
        {
            Q_UNUSED(failedJob);
            qCritical() << "Couldn't check memory contents";
            cb(false, "Error While Correcting Database (Device Disconnected?)");
        });

        jobsQueue.enqueue(repairJobs);
        runAndDequeueJobs();
    });

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Failed scanning the flash memory";
        cb(false, "Couldn't scan the complete memory (Device Disconnected?)");
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

    QByteArray sdata = service.toUtf8();
    sdata.append((char)0);

    jobs->append(new MPCommandJob(this, isDatanode? MPCmd::SET_DATA_SERVICE : MPCmd::CONTEXT,
                                  sdata,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        QVariantMap m = {{ "service", service },
                         { "exists", data[MP_PAYLOAD_FIELD_INDEX] == 1 }};
        jobs->user_data = m;
        return true;
    }));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        //all jobs finished success
        qInfo() << "service_exists success";
        QVariantMap m = jobs->user_data.toMap();
        cb(true, QString(), m["service"].toString(), m["exists"].toBool());
    });

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        qCritical() << "Failed getting data node";
        cb(false, failedJob->getErrorStr(), QString(), false);
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::setMMCredentials(const QJsonArray &creds,
                                MPDeviceProgressCb cbProgress,
                                std::function<void(bool success, QString errstr)> cb)
{
    newAddressesNeededCounter = 0;
    newAddressesReceivedCounter = 0;
    bool packet_send_needed = false;
    AsyncJobs *jobs = new AsyncJobs("Merging credentials changes", this);

    /// TODO: sanitize inputs (or not, as it is done at the mpnode.cpp level)

    /* Look for deleted or changed nodes */
    for (qint32 i = 0; i < creds.size(); i++)
    {
        /* Create object */
        QJsonObject qjobject = creds[i].toObject();

        /* Check format */
        if (qjobject.size() != 6)
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
            for (qint32 j = 0; j < addrArray.size(); j++) { nodeAddr.append(addrArray[j].toInt()); }
            qDebug() << "MMM Save: tackling " << login << " for service " << service << " at address " << nodeAddr.toHex();

            /* Find node in our list */
            MPNode* nodePtr = findNodeWithAddressInList(loginChildNodes, nodeAddr);

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
                newAddressesNeededCounter += 1;

                /* Create new node with null address and virtual address set to our counter value */
                MPNode* newNodePt = new MPNode(QByteArray(MP_NODE_SIZE, 0), this, QByteArray(), newAddressesNeededCounter);
                newNodePt->setType(MPNode::NodeChild);
                loginChildNodes.append(newNodePt);
                newNodePt->setNotDeletedTagged();
                newNodePt->setLogin(login);
                addChildToDB(parentPtr, newNodePt);
                packet_send_needed = true;

                /* Set favorite */
                newNodePt->setFavoriteProperty(favorite);

                /* Finally, change password */
                QStringList changeList;
                changeList << service << login << password;
                mmmPasswordChangeArray.append(changeList);
                qDebug() << "Queing password change as well";
            }
            else if (!nodePtr)
            {
                qCritical() << "Couldn't find" << qjobject["login"].toString() << " for service " << qjobject["service"].toString() << " at address " << nodeAddr.toHex();
                cb(false, "Moolticute Internal Error (SMMC#1)");
                exitMemMgmtMode(true);
                return;
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
                    MPNode* newNode = new MPNode(nodePtr->getNodeData(), this, nodePtr->getAddress(), nodePtr->getVirtualAddress());
                    newNode->setLogin(login);
                    newNode->setNotDeletedTagged();
                    loginChildNodes.append(newNode);
                    removeChildFromDB(parentNodePtr, nodePtr, false);
                    addChildToDB(parentNodePtr, newNode);
                    packet_send_needed = true;

                    /* Set favorite */
                    newNode->setFavoriteProperty(favorite);
                }

                /* Check for changed password */
                if (!password.isEmpty())
                {
                    qDebug() << "Detected password change for" << login << "on" << service;

                    QStringList changeList;
                    changeList << service << login << password;
                    mmmPasswordChangeArray.append(changeList);
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
            if (!curNode->getNotDeletedTagged())
            {
                removeChildFromDB(nodeItem, curNode, true);
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
        qInfo() << "No changes detected";
        cb(true, "No Changes Required");
        exitMemMgmtMode(true);
        return;
    }

    /* Increment db change numbers */
    if (isFw12())
    {
        set_credentialsDbChangeNumber(get_credentialsDbChangeNumber() + 1);
        credentialsDbChangeNumberClone = get_credentialsDbChangeNumber();
        QByteArray updateChangeNumbersPacket = QByteArray();
        updateChangeNumbersPacket.append(get_credentialsDbChangeNumber());
        updateChangeNumbersPacket.append(get_dataDbChangeNumber());
        jobs->append(new MPCommandJob(this, MPCmd::SET_USER_CHANGE_NB, updateChangeNumbersPacket, MPCommandJob::defaultCheckRet));
    }

    emit dbChangeNumbersChanged(get_credentialsDbChangeNumber(), get_dataDbChangeNumber());

    /* Out of pure coding laziness, ask free addresses even if we don't need them */
    loadFreeAddresses(jobs, MPNode::EmptyAddress, false, cbProgress);

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "Received enough free addresses";

        /* We got all the addresses, change virtual addrs for real addrs and finish merging */
        if (newAddressesNeededCounter > 0)
        {
            changeVirtualAddressesToFreeAddresses();
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
        connect(mergeOperations, &AsyncJobs::finished, [=](const QByteArray &data)
        {
            Q_UNUSED(data);
            exitMemMgmtMode(true);
            qInfo() << "Merge operations succeeded!";

            if (mmmPasswordChangeArray.size() > 0)
            {
                AsyncJobs *pwdChangeJobs = new AsyncJobs("Changing passwords...", this);

                /* Create password change jobs */
                for (qint32 i = 0; i < mmmPasswordChangeArray.size(); i++)
                {
                    QByteArray sdata = mmmPasswordChangeArray[i][0].toUtf8();
                    sdata.append((char)0);

                    //First query if context exist
                    pwdChangeJobs->append(new MPCommandJob(this, MPCmd::CONTEXT,
                                                           sdata,
                                                           [=](const QByteArray &data, bool &) -> bool
                    {
                        if (data[MP_PAYLOAD_FIELD_INDEX] != 1)
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

                    QByteArray ldata = mmmPasswordChangeArray[i][1].toUtf8();
                    ldata.append((char)0);

                    pwdChangeJobs->append(new MPCommandJob(this, MPCmd::SET_LOGIN,
                                                  ldata,
                                                  [=](const QByteArray &data, bool &) -> bool
                    {
                        if (data[MP_PAYLOAD_FIELD_INDEX] == 0)
                        {
                            pwdChangeJobs->setCurrentJobError("set_login failed on device");
                            qWarning() << "failed to set login to " << mmmPasswordChangeArray[i][1];
                            return false;
                        }
                        else
                        {
                            qDebug() << "set_login " << mmmPasswordChangeArray[i][1];
                            return true;
                        }
                    }));

                    QByteArray pdata = mmmPasswordChangeArray[i][2].toUtf8();
                    pdata.append((char)0);

                    pwdChangeJobs->append(new MPCommandJob(this, MPCmd::SET_PASSWORD,
                                                   pdata,
                                                   [=](const QByteArray &data, bool &) -> bool
                    {
                        if (data[MP_PAYLOAD_FIELD_INDEX] == 0)
                        {
                            pwdChangeJobs->setCurrentJobError("set_password failed on device");
                            qWarning() << "failed to set_password";
                            return false;
                        }
                        qDebug() << "set_password ok";
                        return true;
                    }));
                }

                connect(pwdChangeJobs, &AsyncJobs::finished, [=](const QByteArray &)
                {
                    cb(true, "Changes Applied to Memory");
                    qInfo() << "Passwords changed!";
                    mmmPasswordChangeArray.clear();
                });

                connect(pwdChangeJobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
                {
                    Q_UNUSED(failedJob);
                    mmmPasswordChangeArray.clear();
                    qCritical() << "Couldn't change passwords";
                    cb(false, "Please Approve Password Changes On The Device");
                });

                jobsQueue.enqueue(pwdChangeJobs);
                runAndDequeueJobs();
            }
            else
            {
                cb(true, "Changes Applied to Memory");
                qInfo() << "No passwords to be changed";
            }
        });
        connect(mergeOperations, &AsyncJobs::failed, [=](AsyncJob *failedJob)
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

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "MMM save failed: couldn't load enough free addresses";
        cb(false, "Couldn't Save Changes (Device Disconnected?)");
        exitMemMgmtMode(true);
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::exportDatabase(QString &encryption, std::function<void(bool success, QString errstr, QByteArray fileData)> cb,
                              MPDeviceProgressCb cbProgress)
{
    /* New job for starting MMM */
    AsyncJobs *jobs = new AsyncJobs("Starting MMM mode for export file generation", this);

    /* Ask device to go into MMM first */
    jobs->append(new MPCommandJob(this, MPCmd::START_MEMORYMGMT, MPCommandJob::defaultCheckRet));

    /* Load flash contents the usual way */
    memMgmtModeReadFlash(jobs, false,
                            cbProgress
                            , true, true, true);

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
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

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Setting device in MMM failed";
        cb(false, "Please Retry and Approve Credential Management", QByteArray());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::importDatabase(const QByteArray &fileData, bool noDelete,
                              std::function<void(bool success, QString errstr)> cb,
                              MPDeviceProgressCb cbProgress)
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
        if (get_status() == Common::UnkownSmartcad)
        {
            AsyncJobs* addcpzjobs = new AsyncJobs("Adding CPZ/CTR...", this);

            /* Query change number */
            addcpzjobs->append(new MPCommandJob(this,
                                          MPCmd::ADD_UNKNOWN_CARD,
                                          unknownCardAddPayload,
                                          [=](const QByteArray &data, bool &) -> bool
            {
                if (data[MP_PAYLOAD_FIELD_INDEX] == 0)
                    return false;
                else
                    return true;
            }));

            connect(addcpzjobs, &AsyncJobs::finished, [=](const QByteArray &data)
            {
                Q_UNUSED(data);
                qInfo() << "CPZ/CTR Added";

                /* Unknown card added, start merging */
                startImportFileMerging(cbProgress, cb, noDelete);
            });

            connect(addcpzjobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
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

void MPDevice::getStoredFiles(std::function<void (bool, QList<QVariantMap>)> cb)
{
    /* New job for starting MMM */
    AsyncJobs *jobs = new AsyncJobs("Starting MMM mode", this);

    /* Ask device to go into MMM first */
    jobs->append(new MPCommandJob(this, MPCmd::START_MEMORYMGMT, MPCommandJob::defaultCheckRet));

    /* Load flash contents the usual way */
    memMgmtModeReadFlash(jobs, false, [](QVariant) {}, false, true, true);

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &data)
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

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Setting device in MMM failed";
        exitMemMgmtMode(false);
        cb(false, QList<QVariantMap>());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}
