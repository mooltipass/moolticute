/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Calaos is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Calaos is distributed in the hope that it will be useful,
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
        sendData(MP_MOOLTIPASS_STATUS, [=](bool success, const QByteArray &data, bool &)
        {
            if (!success)
                return;
            if ((quint8)data.at(1) == MP_MOOLTIPASS_STATUS)
            {
                Common::MPStatus s = (Common::MPStatus)data.at(2);
                if (s != get_status())
                {
                    qDebug() << "received MP_MOOLTIPASS_STATUS: " << (int)data.at(2);

                    if (s == Common::Unlocked)
                    {
                        QTimer::singleShot(10, [=]()
                        {
                            setCurrentDate();
                            loadParameters();
                        });
                    }
                }
                set_status(s);
            }
            else if ((quint8)data.at(1) == MP_PLEASE_RETRY)
            {
                qDebug() << "Please retry received.";
            }
        });
    });

    connect(this, SIGNAL(platformDataRead(QByteArray)), this, SLOT(newDataRead(QByteArray)));
//    connect(this, SIGNAL(platformFailed()), this, SLOT(commandFailed()));
}

MPDevice::~MPDevice()
{
}

void MPDevice::sendData(unsigned char c, const QByteArray &data, MPCommandCb cb)
{
    MPCommand cmd;

    // Prepare MP packet
    cmd.data.append(data.size());
    cmd.data.append(c);
    cmd.data.append(data);
    cmd.cb = std::move(cb);

    commandQueue.enqueue(cmd);

    if (!commandQueue.head().running)
        sendDataDequeue();
}

void MPDevice::sendData(unsigned char cmd, MPCommandCb cb)
{
    sendData(cmd, QByteArray(), std::move(cb));
}

void MPDevice::sendDataDequeue()
{
    if (commandQueue.isEmpty())
        return;

    MPCommand &currentCmd = commandQueue.head();
    currentCmd.running = true;

    // send data with platform code
    //qDebug() << "Platform send command: " << QString("0x%1").arg((quint8)currentCmd.data[1], 2, 16, QChar('0'));
    platformWrite(currentCmd.data);
}

void MPDevice::runAndDequeueJobs()
{
    if (jobsQueue.isEmpty() || currentJobs)
        return;

    currentJobs = jobsQueue.dequeue();
    currentJobs->start();

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
}

bool MPDevice::isJobsQueueBusy()
{
    return currentJobs;
}

void MPDevice::loadParameters()
{
    AsyncJobs *jobs = new AsyncJobs(
                          "Loading device parameters",
                          this);

    jobs->append(new MPCommandJob(this,
                                  MP_VERSION,
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
            isFw12 = v >= 12;
            isMini = match.captured(3) == "_mini";
        }

        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MP_GET_MOOLTIPASS_PARM,
                                  QByteArray(1, KEYBOARD_LAYOUT_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received language: " << (quint8)data.at(2);
        set_keyboardLayout((quint8)data.at(2));
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MP_GET_MOOLTIPASS_PARM,
                                  QByteArray(1, LOCK_TIMEOUT_ENABLE_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received lock timeout enable: " << (quint8)data.at(2);
        set_lockTimeoutEnabled(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MP_GET_MOOLTIPASS_PARM,
                                  QByteArray(1, LOCK_TIMEOUT_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received lock timeout: " << (quint8)data.at(2);
        set_lockTimeout((quint8)data.at(2));
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MP_GET_MOOLTIPASS_PARM,
                                  QByteArray(1, SCREENSAVER_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received screensaver: " << (quint8)data.at(2);
        set_screensaver(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MP_GET_MOOLTIPASS_PARM,
                                  QByteArray(1, USER_REQ_CANCEL_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received userRequestCancel: " << (quint8)data.at(2);
        set_userRequestCancel(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MP_GET_MOOLTIPASS_PARM,
                                  QByteArray(1, USER_INTER_TIMEOUT_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received userInteractionTimeout: " << (quint8)data.at(2);
        set_userInteractionTimeout((quint8)data.at(2));
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MP_GET_MOOLTIPASS_PARM,
                                  QByteArray(1, FLASH_SCREEN_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received flashScreen: " << (quint8)data.at(2);
        set_flashScreen(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MP_GET_MOOLTIPASS_PARM,
                                  QByteArray(1, OFFLINE_MODE_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received offlineMode: " << (quint8)data.at(2);
        set_offlineMode(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MP_GET_MOOLTIPASS_PARM,
                                  QByteArray(1, TUTORIAL_BOOL_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received tutorialEnabled: " << (quint8)data.at(2);
        set_tutorialEnabled(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MP_GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MINI_OLED_CONTRAST_CURRENT_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received screenBrightness: " << (quint8)data.at(2);
        set_screenBrightness((quint8)data.at(2));
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MP_GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MINI_KNOCK_DETECT_ENABLE_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received set_knockEnabled: " << (quint8)data.at(2);
        set_knockEnabled(data.at(2) != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(this,
                                  MP_GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MINI_KNOCK_THRES_PARAM),
                                  [=](const QByteArray &data, bool &) -> bool
    {
        qDebug() << "received knockSensitivity: " << (quint8)data.at(2);
        int v = 1;
        if (data.at(2) == 11) v = 0;
        else if (data.at(2) == 5) v = 2;
        set_knockSensitivity(v);
        return true;
    }));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &data)
    {
        Q_UNUSED(data);
        //data is last result
        //all jobs finished success
        qInfo() << "Finished loading device options";
    });

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Loading option failed";
    });

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

    //qWarning() << "---> Packet data " << " size:" << (quint8)data[0] << " data:" << QString("0x%1").arg((quint8)data[1], 2, 16, QChar('0'));
    if ((quint8)data[1] == MP_DEBUG)
        qWarning() << data;

    if (commandQueue.isEmpty())
    {
        qWarning() << "Command queue is empty!";
        qWarning() << "Packet data " << " size:" << (quint8)data[0] << " data:" << data;
        return;
    }

    MPCommand currentCmd = commandQueue.head();

    bool done = true;
    currentCmd.cb(true, data, done);

    if (done)
    {
        commandQueue.dequeue();
        sendDataDequeue();
    }
}

void MPDevice::updateKeyboardLayout(int lang)
{
    QString logInf = QStringLiteral("Updating keyboad layout: %1").arg(lang);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    QByteArray ba;
    ba.append((quint8)KEYBOARD_LAYOUT_PARAM);
    ba.append((quint8)lang);

    jobs->append(new MPCommandJob(this, MP_SET_MOOLTIPASS_PARM, ba, MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "keyboad layout param set success";
    });
    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed to set keyboad layout";
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::updateLockTimeoutEnabled(bool en)
{
    QString logInf = QStringLiteral("Updating lock timeout enabled: %1").arg(en);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    QByteArray ba;
    ba.append((quint8)LOCK_TIMEOUT_ENABLE_PARAM);
    ba.append((quint8)en);

    jobs->append(new MPCommandJob(this, MP_SET_MOOLTIPASS_PARM, ba, MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "lock timeout param set success";
    });
    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed to set lock timeout enabled";
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::updateLockTimeout(int timeout)
{
    QString logInf = QStringLiteral("Updating lock timeout: %1").arg(timeout);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    if (timeout < 0) timeout = 0;
    if (timeout > 0xFF) timeout = 0xFF;

    QByteArray ba;
    ba.append((quint8)LOCK_TIMEOUT_PARAM);
    ba.append((quint8)timeout);

    jobs->append(new MPCommandJob(this, MP_SET_MOOLTIPASS_PARM, ba, MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "lock timeout param set success";
    });
    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed to set lock timeout";
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::updateScreensaver(bool en)
{
    QString logInf = QStringLiteral("Updating screensaver enabled: %1").arg(en);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    QByteArray ba;
    ba.append((quint8)SCREENSAVER_PARAM);
    ba.append((quint8)en);

    jobs->append(new MPCommandJob(this, MP_SET_MOOLTIPASS_PARM, ba, MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "screensaver param set success";
    });
    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed to set screensaver enabled";
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::updateUserRequestCancel(bool en)
{   
    QString logInf = QStringLiteral("Updating user request cancel enabled: %1").arg(en);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    QByteArray ba;
    ba.append((quint8)USER_REQ_CANCEL_PARAM);
    ba.append((quint8)en);

    jobs->append(new MPCommandJob(this, MP_SET_MOOLTIPASS_PARM, ba, MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "user request cancel param set success";
    });
    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed to set user request cancel enabled";
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::updateUserInteractionTimeout(int timeout)
{   
    QString logInf = QStringLiteral("Updating user interaction timeout: %1").arg(timeout);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    if (timeout < 0) timeout = 0;
    if (timeout > 0xFF) timeout = 0xFF;

    QByteArray ba;
    ba.append((quint8)USER_INTER_TIMEOUT_PARAM);
    ba.append((quint8)timeout);

    jobs->append(new MPCommandJob(this, MP_SET_MOOLTIPASS_PARM, ba, MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "user interaction timeout param set success";
    });
    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed to set user interaction timeout";
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::updateFlashScreen(bool en)
{
    QString logInf = QStringLiteral("Updating flash screen enabled: %1").arg(en);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    QByteArray ba;
    ba.append((quint8)FLASH_SCREEN_PARAM);
    ba.append((quint8)en);

    jobs->append(new MPCommandJob(this, MP_SET_MOOLTIPASS_PARM, ba, MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "flash screen param set success";
    });
    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed to set flash screen enabled";
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::updateOfflineMode(bool en)
{
    QString logInf = QStringLiteral("Updating offline enabled: %1").arg(en);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    QByteArray ba;
    ba.append((quint8)OFFLINE_MODE_PARAM);
    ba.append((quint8)en);

    jobs->append(new MPCommandJob(this, MP_SET_MOOLTIPASS_PARM, ba, MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "Offline param set success";
    });
    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed to set offline enabled";
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::updateTutorialEnabled(bool en)
{
    QString logInf = QStringLiteral("Updating tutorial enabled: %1").arg(en);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    QByteArray ba;
    ba.append((quint8)TUTORIAL_BOOL_PARAM);
    ba.append((quint8)en);

    jobs->append(new MPCommandJob(this, MP_SET_MOOLTIPASS_PARM, ba, MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "Tutorial param set success";
    });
    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed to set tutorial enabled";
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::updateScreenBrightness(int bval) //In percent
{
    QString logInf = QStringLiteral("Updating screen brightness: %1").arg(bval);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    QByteArray ba;
    ba.append((quint8)MINI_OLED_CONTRAST_CURRENT_PARAM);
    ba.append((quint8)(bval));

    jobs->append(new MPCommandJob(this, MP_SET_MOOLTIPASS_PARM, ba, MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "Knock enabled set success";
    });
    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed to set knock enabled";
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::updateKnockEnabled(bool en)
{
    QString logInf = QStringLiteral("Updating knock enabled: %1").arg(en);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    QByteArray ba;
    ba.append((quint8)MINI_KNOCK_DETECT_ENABLE_PARAM);
    ba.append(en);

    jobs->append(new MPCommandJob(this, MP_SET_MOOLTIPASS_PARM, ba, MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "Knock enabled set success";
    });
    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed to set knock enabled";
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::updateKnockSensitivity(int s) // 0-low, 1-medium, 2-high
{
    QString logInf = QStringLiteral("Update knock sensitivity: %1").arg(s);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    quint8 v = 8;
    if (s == 0) v = 11;
    else if (s == 2) v = 5;

    QByteArray ba;
    ba.append((quint8)MINI_KNOCK_THRES_PARAM);
    ba.append(v);

    jobs->append(new MPCommandJob(this, MP_SET_MOOLTIPASS_PARM, ba, MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "Knock sensitivity set success";
    });
    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed to set sensitivity";
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::startMemMgmtMode()
{
    /* Start MMM here, and load all memory data from the device
     * (favorites, nodes, etc...)
     */

    if (get_memMgmtMode()) return;

    AsyncJobs *jobs = new AsyncJobs("Starting MMM mode", this);

    //Ask device to go into MMM first
    jobs->append(new MPCommandJob(this, MP_START_MEMORYMGMT, MPCommandJob::defaultCheckRet));

    //Get CTR value
    jobs->append(new MPCommandJob(this, MP_GET_CTRVALUE,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[0] == 1) return false;
        ctrValue = data.mid(2, data[0]);
        qDebug() << "CTR value is: " << ctrValue;
        return true;
    }));

    //Get CPZ and CTR value
    jobs->append(new MPCommandJob(this, MP_GET_CARD_CPZ_CTR,
                                  [=](const QByteArray &data, bool &done) -> bool
    {
        if ((quint8)data[1] == MP_CARD_CPZ_CTR_PACKET)
        {
            done = false;
            QByteArray cpz = data.mid(2, data[0]);
            qDebug() << "CPZ packet: " << cpz;
            cpzCtrValue.append(cpz);
        }
        return true;
    }));

    //Add jobs for favorites
    favoritesAddrs.clear();
    for (int i = 0;i < MOOLTIPASS_FAV_MAX;i++)
    {
        jobs->append(new MPCommandJob(this, MP_GET_FAVORITE,
                                      QByteArray(1, (quint8)i),
                                      [=](const QByteArray &, QByteArray &) -> bool
        {
            if (i == 0) qInfo() << "Loading favorites...";
            return true;
        },
                                      [=](const QByteArray &data, bool &) -> bool
        {
            if (data[0] == 1) return false;
            favoritesAddrs.append(data.mid(2, 4));
            return true;
        }));
    }

    qDeleteAll(loginNodes);
    loginNodes.clear();

    //Get parent node start address
    jobs->append(new MPCommandJob(this, MP_GET_STARTING_PARENT,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[0] == 1) return false;
        QByteArray addr = data.mid(2, data[0]);

        //if parent address is not null, load nodes
        if (addr != MPNode::EmptyAddress)
        {
            qInfo() << "Loading parent nodes...";
            loadLoginNode(jobs, addr);
        }
        else
            qInfo() << "No parent nodes to load.";

        return true;
    }));

    qDeleteAll(dataNodes);
    dataNodes.clear();

    //Get parent data node start address
    jobs->append(new MPCommandJob(this, MP_GET_DN_START_PARENT,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[0] == 1) return false;
        QByteArray addr = data.mid(2, data[0]);

        if (addr != MPNode::EmptyAddress)
        {
            qInfo() << "Loading parent data nodes...";
            loadDataNode(jobs, addr);
        }
        else
            qInfo() << "No parent data nodes to load.";

        return true;
    }));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &data)
    {
        Q_UNUSED(data);
        //data is last result
        //all jobs finished success

        qInfo() << "Mem management mode enabled";
        force_memMgmtMode(true);
    });

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Setting device in MMM failed";

        //Clear remaining data
        ctrValue.clear();
        cpzCtrValue.clear();
        qDeleteAll(loginNodes);
        loginNodes.clear();
        qDeleteAll(dataNodes);
        dataNodes.clear();
        favoritesAddrs.clear();

        force_memMgmtMode(false);
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::loadLoginNode(AsyncJobs *jobs, const QByteArray &address)
{
    MPNode *pnode = new MPNode(this);
    loginNodes.append(pnode);

    qDebug() << "Loading cred parent node at address: " << address;

    jobs->append(new MPCommandJob(this, MP_READ_FLASH_NODE,
                                  address,
                                  [=](const QByteArray &data, bool &done) -> bool
    {
        if ((quint8)data[0] <= 1) return false;

        pnode->appendData(data.mid(2, data[0]));

        //Continue to read data until the node is fully received
        if (!pnode->isValid())
            done = false;
        else
        {
            //Node is loaded
            qDebug() << "Parent node loaded: " << pnode->getService();
            if (pnode->getStartChildAddress() != MPNode::EmptyAddress)
            {
                qDebug() << "Loading child nodes...";
                loadLoginChildNode(jobs, pnode, pnode->getStartChildAddress());
            }
            else
                qDebug() << "Parent does not have childs.";

            //Load next parent
            if (pnode->getNextParentAddress() != MPNode::EmptyAddress)
                loadLoginNode(jobs, pnode->getNextParentAddress());
        }

        return true;
    }));
}

void MPDevice::loadLoginChildNode(AsyncJobs *jobs, MPNode *parent, const QByteArray &address)
{
    MPNode *cnode = new MPNode(this);
    parent->appendChild(cnode);

    qDebug() << "Loading cred child node at address: " << address;

    jobs->prepend(new MPCommandJob(this, MP_READ_FLASH_NODE,
                                  address,
                                  [=](const QByteArray &data, bool &done) -> bool
    {
        if ((quint8)data[0] <= 1) return false;

        cnode->appendData(data.mid(2, data[0]));

        //Continue to read data until the node is fully received
        if (!cnode->isValid())
            done = false;
        else
        {
            //Node is loaded
            qDebug() << "Child node loaded: " << cnode->getLogin();

            //Load next child
            if (cnode->getNextChildAddress() != MPNode::EmptyAddress)
                loadLoginChildNode(jobs, parent, cnode->getNextChildAddress());
        }

        return true;
    }));
}

void MPDevice::loadDataNode(AsyncJobs *jobs, const QByteArray &address)
{
    MPNode *pnode = new MPNode(this);
    dataNodes.append(pnode);

    qDebug() << "Loading data parent node at address: " << address;

    jobs->append(new MPCommandJob(this, MP_READ_FLASH_NODE,
                                  address,
                                  [=](const QByteArray &data, bool &done) -> bool
    {
        if ((quint8)data[0] <= 1) return false;

        pnode->appendData(data.mid(2, data[0]));

        //Continue to read data until the node is fully received
        if (!pnode->isValid())
            done = false;
        else
        {
            //Node is loaded
            qDebug() << "Parent data node loaded: " << pnode->getService();

            //Load data child
            if (pnode->getStartChildAddress() != MPNode::EmptyAddress)
            {
                qDebug() << "Loading data child nodes...";
                loadDataChildNode(jobs, pnode, pnode->getStartChildAddress());
            }
            else
                qDebug() << "Parent data node does not have childs.";

            //Load next parent
            if (pnode->getNextParentAddress() != MPNode::EmptyAddress)
                loadDataNode(jobs, pnode->getNextParentAddress());
        }

        return true;
    }));
}

void MPDevice::loadDataChildNode(AsyncJobs *jobs, MPNode *parent, const QByteArray &address)
{
    MPNode *cnode = new MPNode(this);
    parent->appendChildData(cnode);

    qDebug() << "Loading data child node at address: " << address;

    jobs->prepend(new MPCommandJob(this, MP_READ_FLASH_NODE,
                                  address,
                                  [=](const QByteArray &data, bool &done) -> bool
    {
        if ((quint8)data[0] <= 1) return false;

        cnode->appendData(data.mid(2, data[0]));

        //Continue to read data until the node is fully received
        if (!cnode->isValid())
            done = false;
        else
        {
            //Node is loaded
            qDebug() << "Child data node loaded";

            //Load next child
            if (cnode->getNextChildDataAddress() != MPNode::EmptyAddress)
                loadDataChildNode(jobs, parent, cnode->getNextChildDataAddress());
        }

        return true;
    }));
}

void MPDevice::exitMemMgmtMode()
{
    if (!get_memMgmtMode()) return;

    AsyncJobs *jobs = new AsyncJobs("Exiting MMM", this);

    jobs->append(new MPCommandJob(this, MP_END_MEMORYMGMT, MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        //data is last result
        //all jobs finished success

        qInfo() << "MMM exit ok";

        ctrValue.clear();
        cpzCtrValue.clear();
        qDeleteAll(loginNodes);
        loginNodes.clear();
        qDeleteAll(dataNodes);
        dataNodes.clear();
        favoritesAddrs.clear();

        force_memMgmtMode(false);
    });

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qCritical() << "Failed to exit MMM";

        ctrValue.clear();
        cpzCtrValue.clear();
        qDeleteAll(loginNodes);
        loginNodes.clear();
        qDeleteAll(dataNodes);
        dataNodes.clear();
        favoritesAddrs.clear();

        force_memMgmtMode(false);
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::setCurrentDate()
{
    AsyncJobs *jobs = new AsyncJobs("Send date to device", this);

    jobs->append(new MPCommandJob(this, MP_SET_DATE,
                                  [=](const QByteArray &, QByteArray &data_to_send) -> bool
    {
        data_to_send.clear();
        data_to_send.append(Common::dateToBytes(QDate::currentDate()));

        qDebug() << "Sending current date: " <<
                    QString("0x%1").arg((quint8)data_to_send[0], 2, 16, QChar('0')) <<
                    QString("0x%1").arg((quint8)data_to_send[1], 2, 16, QChar('0'));

        return true;
    },
                                  MPCommandJob::defaultCheckRet));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        qInfo() << "Date set success";
    });
    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *)
    {
        qWarning() << "Failed to set date on device";
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::cancelUserRequest(const QString &reqid)
{
    // send data with platform code
    // This command is used to cancel a request.
    // As the request may block the sending queue, we directly send the command
    // and bypass the queue.

    if (!isFw12)
    {
        qDebug() << "cancelUserRequest not supported for fw < 1.2";
        return;
    }

    qInfo() << "cancel user request (reqid: " << reqid << ")";

    if (currentJobs->getJobsId() == reqid)
    {
        qInfo() << "request_id match current one. Cancel current request";

        QByteArray ba;
        ba.append((char)0);
        ba.append(MP_CANCEL_USER_REQUEST);

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

void MPDevice::askPassword(const QString &service, const QString &login, const QString &fallback_service, const QString &reqid,
                        std::function<void(bool success, QString errstr, const QString &_service, const QString &login, const QString &pass)> cb)
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

    jobs->append(new MPCommandJob(this, MP_CONTEXT,
                                  sdata,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[2] != 1)
        {
            if (!fallback_service.isEmpty())
            {
                QByteArray fsdata = fallback_service.toUtf8();
                fsdata.append((char)0);
                jobs->prepend(new MPCommandJob(this, MP_CONTEXT,
                                              fsdata,
                                              [=](const QByteArray &data, bool &) -> bool
                {
                    if (data[2] != 1)
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

    jobs->append(new MPCommandJob(this, MP_GET_LOGIN,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[2] == 0 && !login.isEmpty())
        {
            jobs->setCurrentJobError("credential access refused by user");
            return false;
        }

        QString l = data.mid(2, data[0]);
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

    jobs->append(new MPCommandJob(this, MP_GET_PASSWORD,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[2] == 0)
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
        QString pass = data.mid(2, data[0]);

        QVariantMap m = jobs->user_data.toMap();
        cb(true, QString(), m["service"].toString(), m["login"].toString(), pass);
    });

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        qCritical() << "Failed getting password";
        cb(false, failedJob->getErrorStr(), QString(), QString(), QString());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

void MPDevice::getRandomNumber(std::function<void(bool success, QString errstr, const QByteArray &nums)> cb)
{
    AsyncJobs *jobs = new AsyncJobs("Get random numbers from device", this);

    jobs->append(new MPCommandJob(this, MP_GET_RANDOM_NUMBER, QByteArray()));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &data)
    {
        //data is last result
        //all jobs finished success

        qInfo() << "Random numbers generated ok";
        cb(true, QString(), data);
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

    quint8 cmdAddCtx = isDataNode?MP_ADD_DATA_SERVICE:MP_ADD_CONTEXT;
    quint8 cmdSelectCtx = isDataNode?MP_SET_DATA_SERVICE:MP_CONTEXT;

    //Create context
    jobs->prepend(new MPCommandJob(this, cmdAddCtx,
                  sdata,
                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[2] != 1)
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
        if (data[2] != 1)
        {
            qWarning() << "Failed to select new context";
            jobs->setCurrentJobError("unable to selected context on device");
            return false;
        }
        qDebug() << "set_context " << service;
        return true;
    }), 0);
}

void MPDevice::setCredential(const QString &service, const QString &login,
                             const QString &pass, const QString &description,
                             std::function<void(bool success, QString errstr)> cb)
{
    if (service.isEmpty() ||
        pass.isEmpty())
    {
        qWarning() << "context or pass is empty.";
        cb(false, "context or password is empty");
        return;
    }

    QString logInf = QStringLiteral("Adding/Changing credential for service: %1 login: %2")
                     .arg(service)
                     .arg(login);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    QByteArray sdata = service.toUtf8();
    sdata.append((char)0);

    //First query if context exist
    jobs->append(new MPCommandJob(this, MP_CONTEXT,
                                  sdata,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[2] != 1)
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

    jobs->append(new MPCommandJob(this, MP_SET_LOGIN,
                                  ldata,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[2] == 0)
        {
            jobs->setCurrentJobError("set_login failed on device");
            qWarning() << "failed to set login to " << login;
            return false;
        }
        qDebug() << "set_login " << login;
        return true;
    }));

    if (isFw12)
    {
        QByteArray ddata = description.toUtf8();
        ddata.append((char)0);

        //Set description should be done right after set login
        jobs->append(new MPCommandJob(this, MP_SET_DESCRIPTION,
                                      ddata,
                                      [=](const QByteArray &data, bool &) -> bool
        {
            if (data[2] == 0)
            {
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

    jobs->append(new MPCommandJob(this, MP_CHECK_PASSWORD,
                                  pdata,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[2] != 1)
        {
            //Password does not match, update it
            jobs->prepend(new MPCommandJob(this, MP_SET_PASSWORD,
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

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        //all jobs finished success
        qInfo() << "set_credential success";
        cb(true, QString());
    });

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        qCritical() << "Failed adding new credential";
        cb(false, failedJob->getErrorStr());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}

bool MPDevice::getDataNodeCb(AsyncJobs *jobs, const QByteArray &data, bool &)
{
    using namespace std::placeholders;

    qDebug() << "getDataNodeCb data size: " << ((quint8)data[0]) << "  " << ((quint8)data[1]) << "  " << ((quint8)data[2]);

    if (data[0] == 1 && //data size is 1
        data[2] == 0)   //value is 0 means end of data
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

    if (data[0] != 0)
    {
        QVariantMap m = jobs->user_data.toMap();
        QByteArray ba = m["data"].toByteArray();
        ba.append(data.mid(2, (int)data.at(0)));
        m["data"] = ba;
        jobs->user_data = m;

        //ask for the next 32bytes packet
        //bind to a member function of MPDevice, to be able to loop over until with got all the data
        jobs->append(new MPCommandJob(this, MP_READ_32B_IN_DN,
                                      std::bind(&MPDevice::getDataNodeCb, this, jobs, _1, _2)));
    }
    return true;
}

void MPDevice::getDataNode(const QString &service, const QString &fallback_service, const QString &reqid,
                           std::function<void(bool success, QString errstr, QString serv, QByteArray rawData)> cb)
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

    jobs->append(new MPCommandJob(this, MP_SET_DATA_SERVICE,
                                  sdata,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[2] != 1)
        {
            if (!fallback_service.isEmpty())
            {
                QByteArray fsdata = fallback_service.toUtf8();
                fsdata.append((char)0);
                jobs->prepend(new MPCommandJob(this, MP_SET_DATA_SERVICE,
                                              fsdata,
                                              [=](const QByteArray &data, bool &) -> bool
                {
                    if (data[2] != 1)
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
    jobs->append(new MPCommandJob(this, MP_READ_32B_IN_DN,
                                  std::bind(&MPDevice::getDataNodeCb, this, jobs, _1, _2)));

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

bool MPDevice::setDataNodeCb(AsyncJobs *jobs, int current, const QByteArray &data, bool &)
{
    using namespace std::placeholders;

    qDebug() << "setDataNodeCb data current: " << current;

    if (data[2] == 0)
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

    //send 32bytes packet
    //bind to a member function of MPDevice, to be able to loop over until with got all the data
    jobs->append(new MPCommandJob(this, MP_WRITE_32B_IN_DN,
                                  packet,
                                  std::bind(&MPDevice::setDataNodeCb, this, jobs, current + MOOLTIPASS_BLOCK_SIZE, _1, _2)));

    return true;
}

void MPDevice::setDataNode(const QString &service, const QByteArray &nodeData, const QString &reqid,
                           std::function<void(bool success, QString errstr)> cb)
{
    if (service.isEmpty())
    {
        qWarning() << "context is empty.";
        cb(false, "context is empty");
        return;
    }

    QString logInf = QStringLiteral("Set data node for service: %1 reqid: %2")
                     .arg(service)
                     .arg(reqid);

    AsyncJobs *jobs;
    if (reqid.isEmpty())
        jobs = new AsyncJobs(logInf, this);
    else
        jobs = new AsyncJobs(logInf, reqid, this);

    QByteArray sdata = service.toUtf8();
    sdata.append((char)0);

    jobs->append(new MPCommandJob(this, MP_SET_DATA_SERVICE,
                                  sdata,
                                  [=](const QByteArray &data, bool &) -> bool
    {
        if (data[2] != 1)
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
    currentDataNode.resize(4);
    qToBigEndian(nodeData.size(), (quint8 *)currentDataNode.data());
    currentDataNode.append(nodeData);

    //first packet
    QByteArray firstPacket;
    char eod = (nodeData.size() <= MOOLTIPASS_BLOCK_SIZE)?1:0;
    firstPacket.append(eod);
    firstPacket.append(currentDataNode.mid(0, MOOLTIPASS_BLOCK_SIZE));
    firstPacket.resize(MOOLTIPASS_BLOCK_SIZE + 1);

    //send the first 32bytes packet
    //bind to a member function of MPDevice, to be able to loop over until with got all the data
    jobs->append(new MPCommandJob(this, MP_WRITE_32B_IN_DN,
                                  firstPacket,
                                  std::bind(&MPDevice::setDataNodeCb, this, jobs, MOOLTIPASS_BLOCK_SIZE, _1, _2)));

    connect(jobs, &AsyncJobs::finished, [=](const QByteArray &)
    {
        //all jobs finished success
        qInfo() << "set_data_node success";
        cb(true, QString());
    });

    connect(jobs, &AsyncJobs::failed, [=](AsyncJob *failedJob)
    {
        qCritical() << "Failed writing data node";
        cb(false, failedJob->getErrorStr());
    });

    jobsQueue.enqueue(jobs);
    runAndDequeueJobs();
}
