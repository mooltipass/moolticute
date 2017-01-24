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
#ifndef MPDEVICE_H
#define MPDEVICE_H

#include <QObject>
#include "Common.h"
#include "MooltipassCmds.h"
#include "QtHelper.h"
#include "AsyncJobs.h"
#include "MPNode.h"

typedef std::function<void(bool success, const QByteArray &data, bool &done)> MPCommandCb;

class MPCommand
{
public:
    QByteArray data;
    MPCommandCb cb;
    bool running = false;
};

class MPDevice: public QObject
{
    Q_OBJECT
    QT_WRITABLE_PROPERTY(Common::MPStatus, status, Common::UnknownStatus)
    QT_WRITABLE_PROPERTY(int, keyboardLayout, 0)
    QT_WRITABLE_PROPERTY(bool, lockTimeoutEnabled, false)
    QT_WRITABLE_PROPERTY(int, lockTimeout, 0)
    QT_WRITABLE_PROPERTY(bool, screensaver, false)
    QT_WRITABLE_PROPERTY(bool, userRequestCancel, false)
    QT_WRITABLE_PROPERTY(int, userInteractionTimeout, 0)
    QT_WRITABLE_PROPERTY(bool, flashScreen, false)
    QT_WRITABLE_PROPERTY(bool, offlineMode, false)
    QT_WRITABLE_PROPERTY(bool, tutorialEnabled, false)
    QT_WRITABLE_PROPERTY(bool, memMgmtMode, false)
    QT_WRITABLE_PROPERTY(int, flashMbSize, 0)
    QT_WRITABLE_PROPERTY(QString, hwVersion, QString())

    //MP Mini only
    QT_WRITABLE_PROPERTY(int, screenBrightness, 0) //51-20%, 89-35%, 128-50%, 166-65%, 204-80%, 255-100%
    QT_WRITABLE_PROPERTY(bool, knockEnabled, false)
    QT_WRITABLE_PROPERTY(int, knockSensitivity, 0) // 0-low, 1-medium, 2-high

public:
    MPDevice(QObject *parent);
    virtual ~MPDevice();

    /* Send a command with data to the device */
    void sendData(unsigned char cmd, const QByteArray &data = QByteArray(), MPCommandCb cb = [](bool, const QByteArray &, bool &){});
    void sendData(unsigned char cmd, MPCommandCb cb = [](bool, const QByteArray &, bool &){});

    void updateKeyboardLayout(int lang);
    void updateLockTimeoutEnabled(bool en);
    void updateLockTimeout(int timeout);
    void updateScreensaver(bool en);
    void updateUserRequestCancel(bool en);
    void updateUserInteractionTimeout(int timeout);
    void updateFlashScreen(bool en);
    void updateOfflineMode(bool en);
    void updateTutorialEnabled(bool en);

    //MP Mini only
    void updateScreenBrightness(int bval); //51-20%, 89-35%, 128-50%, 166-65%, 204-80%, 255-100%
    void updateKnockEnabled(bool en);
    void updateKnockSensitivity(int s); // 0-low, 1-medium, 2-high

    //mem mgmt mode
    void startMemMgmtMode();
    void exitMemMgmtMode();
    bool checkLoadedNodes();

    //reload parameters from MP
    void loadParameters();

    //Send current date to MP
    void setCurrentDate();

    //Get database change numbers
    void getChangeNumbers();

    //Ask a password for specified service/login to MP
    void getCredential(const QString &service, const QString &login, const QString &fallback_service, const QString &reqid,
                       std::function<void(bool success, QString errstr, const QString &_service, const QString &login, const QString &pass, const QString &desc)> cb);

    //Add or Set service/login/pass/desc in MP
    void setCredential(const QString &service, const QString &login,
                       const QString &pass, const QString &description, bool setDesc,
                       std::function<void(bool success, QString errstr)> cb);

    //get 32 random bytes from device
    void getRandomNumber(std::function<void(bool success, QString errstr, const QByteArray &nums)> cb);

    //Send a cancel request to device
    void cancelUserRequest(const QString &reqid);

    //Request for a raw data node from the device
    void getDataNode(const QString &service, const QString &fallback_service, const QString &reqid,
                     std::function<void(bool success, QString errstr, QString service, QByteArray rawData)> cb,
                     std::function<void(int total, int current)> cbProgress);

    //Set data to a context on the device
    void setDataNode(const QString &service, const QByteArray &nodeData, const QString &reqid,
                     std::function<void(bool success, QString errstr)> cb,
                     std::function<void(int total, int current)> cbProgress);

    //After successfull mem mgmt mode, clients can query data
    QList<MPNode *> &getLoginNodes() { return loginNodes; }
    QList<MPNode *> &getDataNodes() { return dataNodes; }

    //true if device is a mini
    bool isMini() { return isMiniFlag; }
    //true if device fw version is at least 1.2
    bool isFw12() { return isFw12Flag; }

signals:
    /* Signal emited by platform code when new data comes from MP */
    /* A signal is used for platform code that uses a dedicated thread */
    void platformDataRead(const QByteArray &data);

    /* the command has failed in platform code */
    void platformFailed();

private slots:
    void newDataRead(const QByteArray &data);
    void commandFailed();
    void sendDataDequeue(); //execute commands from the command queue
    void runAndDequeueJobs(); //execute AsyncJobs from the jobs queues

private:
    /* Platform function for starting a read, should be implemented in platform class */
    virtual void platformRead() {}

    /* Platform function for writing data, should be implemented in platform class */
    virtual void platformWrite(const QByteArray &data) { Q_UNUSED(data); }

    void loadLoginNode(AsyncJobs *jobs, const QByteArray &address);
    void loadLoginChildNode(AsyncJobs *jobs, MPNode *parent, MPNode *parentClone, const QByteArray &address);
    void loadDataNode(AsyncJobs *jobs, const QByteArray &address);
    void loadDataChildNode(AsyncJobs *jobs, MPNode *parent, const QByteArray &address);

    void createJobAddContext(const QString &service, AsyncJobs *jobs, bool isDataNode = false);

    bool getDataNodeCb(AsyncJobs *jobs,
                       std::function<void(int total, int current)> cbProgress,
                       const QByteArray &data, bool &done);
    bool setDataNodeCb(AsyncJobs *jobs, int current,
                       std::function<void(int total, int current)> cbProgress,
                       const QByteArray &data, bool &done);

    // Functions added by mathieu for MMM
    MPNode *findNodeWithAddressInList(QList<MPNode *> list, const QByteArray &address);
    void tagPointedNodes();

    // Generate save packets
    bool generateSavePackets();

    void updateParam(quint8 param, bool en);
    void updateParam(quint8 param, int val);

    //timer that asks status
    QTimer *statusTimer = nullptr;

    //command queue
    QQueue<MPCommand> commandQueue;

    // Values loaded when needed (e.g. mem mgmt mode)
    QByteArray ctrValue;
    QByteArray startNode;
    QByteArray startDataNode;
    QList<QByteArray> cpzCtrValue;
    QList<QByteArray> favoritesAddrs;
    QList<MPNode *> loginNodes;         //list of all parent nodes for credentials
    QList<MPNode *> loginChildNodes;    //list of all parent nodes for credentials
    QList<MPNode *> dataNodes;          //list of all parent nodes for data nodes

    // Clones of these values, used when modifying them in MMM
    QByteArray ctrValueClone;
    QByteArray startNodeClone;
    QList<QByteArray> cpzCtrValueClone;
    QList<QByteArray> favoritesAddrsClone;
    QList<MPNode *> loginNodesClone;         //list of all parent nodes for credentials
    QList<MPNode *> loginChildNodesClone;    //list of all parent nodes for credentials
    QList<MPNode *> dataNodesClone;          //list of all parent nodes for data nodes

    bool isMiniFlag = false;            // true if fw is mini
    bool isFw12Flag = false;            // true if fw is at least v1.2
    quint32 serialNumber;               // serial number if firmware is above 1.2
    quint8 credentialsDbChangeNumber;   // credentials db change number
    quint8 dataDbChangeNumber;          // data db change number

    //this queue is used to put jobs list in a wait
    //queue. it prevents other clients to query something
    //when an AsyncJobs is currently running.
    //An AsyncJobs can also be removed if it was not started (using cancelUserRequest for example)
    //All AsyncJobs does have an <id>
    QQueue<AsyncJobs *> jobsQueue;
    AsyncJobs *currentJobs = nullptr;
    bool isJobsQueueBusy(); //helper to check if something is already running

    //this is a cache for data upload
    QByteArray currentDataNode;
};

#endif // MPDEVICE_H
