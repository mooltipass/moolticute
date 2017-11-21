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
#ifndef MPDEVICE_H
#define MPDEVICE_H

#include <QObject>
#include "Common.h"
#include "MooltipassCmds.h"
#include "QtHelper.h"
#include "AsyncJobs.h"
#include "MPNode.h"
#include "FilesCache.h"

typedef std::function<void(bool success, const QByteArray &data, bool &done)> MPCommandCb;
typedef std::function<void(const QVariantMap &data)> MPDeviceProgressCb;
/* Example usage of the above function
 * MPDeviceProgressCb cb;
 * QVariantMap progressData = { {"total", 100},
 *                      {"current", 2},
 *                      {"msg", "message with %1 arguments: %2 %3"},
 *                      {"msg_args", QVariantList({1, "23", "23423"})}};
 * cb(data)
*/

class MPCommand
{
public:
    QByteArray data;
    MPCommandCb cb;
    bool running = false;

    QTimer *timerTimeout = nullptr;
    int retry = CMD_MAX_RETRY;

    bool checkReturn = true;
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

    QT_WRITABLE_PROPERTY(bool, keyAfterLoginSendEnable, false)
    QT_WRITABLE_PROPERTY(int, keyAfterLoginSend, 0)
    QT_WRITABLE_PROPERTY(bool, keyAfterPassSendEnable, false)
    QT_WRITABLE_PROPERTY(int, keyAfterPassSend, 0)
    QT_WRITABLE_PROPERTY(bool, delayAfterKeyEntryEnable, false)
    QT_WRITABLE_PROPERTY(int, delayAfterKeyEntry, 0)



    //MP Mini only
    QT_WRITABLE_PROPERTY(int, screenBrightness, 0) //51-20%, 89-35%, 128-50%, 166-65%, 204-80%, 255-100%
    QT_WRITABLE_PROPERTY(bool, knockEnabled, false)
    QT_WRITABLE_PROPERTY(int, knockSensitivity, 0) // 0-low, 1-medium, 2-high
    QT_WRITABLE_PROPERTY(bool, randomStartingPin, false)
    QT_WRITABLE_PROPERTY(bool, hashDisplay, false)
    QT_WRITABLE_PROPERTY(int, lockUnlockMode, 0)

    QT_WRITABLE_PROPERTY(quint32, serialNumber, 0)              // serial number if firmware is above 1.2
    QT_WRITABLE_PROPERTY(quint8, credentialsDbChangeNumber, 0)  // credentials db change number
    QT_WRITABLE_PROPERTY(quint8, dataDbChangeNumber, 0)         // data db change number
    QT_WRITABLE_PROPERTY(QByteArray, cardCPZ, QByteArray())     // Card CPZ

    QT_WRITABLE_PROPERTY(qint64, uid, -1)

public:
    MPDevice(QObject *parent);
    virtual ~MPDevice();

    /* Send a command with data to the device */
    void sendData(MPCmd::Command cmd, const QByteArray &data = QByteArray(), quint32 timeout = CMD_DEFAULT_TIMEOUT, MPCommandCb cb = [](bool, const QByteArray &, bool &){}, bool checkReturn = true);
    void sendData(MPCmd::Command cmd, quint32 timeout, MPCommandCb cb);
    void sendData(MPCmd::Command cmd, MPCommandCb cb = [](bool, const QByteArray &, bool &){});

    void updateKeyboardLayout(int lang);
    void updateLockTimeoutEnabled(bool en);
    void updateLockTimeout(int timeout);
    void updateScreensaver(bool en);
    void updateUserRequestCancel(bool en);
    void updateUserInteractionTimeout(int timeout);
    void updateFlashScreen(bool en);
    void updateOfflineMode(bool en);
    void updateTutorialEnabled(bool en);
    void updateKeyAfterLoginSendEnable(bool en);
    void updateKeyAfterLoginSend(int value);
    void updateKeyAfterPassSendEnable(bool en);
    void updateKeyAfterPassSend(int value);
    void updateDelayAfterKeyEntryEnable(bool en);
    void updateDelayAfterKeyEntry(int val);


    //MP Mini only
    void updateScreenBrightness(int bval); //51-20%, 89-35%, 128-50%, 166-65%, 204-80%, 255-100%
    void updateKnockEnabled(bool en);
    void updateKnockSensitivity(int s); // 0-low, 1-medium, 2-high
    void updateRandomStartingPin(bool);
    void updateHashDisplay(bool);
    void updateLockUnlockMode(int);

    void getUID(const QByteArray & key);

    //mem mgmt mode
    //cbFailure is used to propagate an error to clients when entering mmm
    void startMemMgmtMode(bool wantData,
                          MPDeviceProgressCb cbProgress,
                          std::function<void(bool success, int errCode, QString errMsg)> cb);
    void exitMemMgmtMode(bool setMMMBool = true);
    void startIntegrityCheck(std::function<void(bool success, QString errstr)> cb,
                             MPDeviceProgressCb cbProgress);

    //reload parameters from MP
    void loadParameters();

    //Send current date to MP
    void setCurrentDate();

    //Get database change numbers
    void getChangeNumbers();

    //Get current card CPZ
    void getCurrentCardCPZ();

    //Ask a password for specified service/login to MP
    void getCredential(const QString &service, const QString &login, const QString &fallback_service, const QString &reqid,
                       std::function<void(bool success, QString errstr, const QString &_service, const QString &login, const QString &pass, const QString &desc)> cb);

    //Add or Set service/login/pass/desc in MP
    void setCredential(const QString &service, const QString &login,
                       const QString &pass, const QString &description, bool setDesc,
                       std::function<void(bool success, QString errstr)> cb);

    //Delete credential in MMM and leave
    void delCredentialAndLeave(const QString &service, const QString &login,
                               MPDeviceProgressCb cbProgress,
                               std::function<void(bool success, QString errstr)> cb);

    //get 32 random bytes from device
    void getRandomNumber(std::function<void(bool success, QString errstr, const QByteArray &nums)> cb);

    //Send a cancel request to device
    void cancelUserRequest(const QString &reqid);

    //Request for a raw data node from the device
    void getDataNode(const QString &service, const QString &fallback_service, const QString &reqid,
                     std::function<void(bool success, QString errstr, QString service, QByteArray rawData)> cb,
                     MPDeviceProgressCb cbProgress);

    //Set data to a context on the device
    void setDataNode(const QString &service, const QByteArray &nodeData,
                     std::function<void(bool success, QString errstr)> cb,
                     MPDeviceProgressCb cbProgress);

    //Delete a data context from the device
    void deleteDataNodesAndLeave(const QStringList &services,
                                 std::function<void(bool success, QString errstr)> cb,
                                 MPDeviceProgressCb cbProgress);

    //Check is credential/data node exists
    void serviceExists(bool isDatanode, const QString &service, const QString &reqid,
                       std::function<void(bool success, QString errstr, QString service, bool exists)> cb);

    //Set full list of credentials in MMM
    void setMMCredentials(const QJsonArray &creds, MPDeviceProgressCb cbProgress,
                          std::function<void(bool success, QString errstr)> cb);

    //Export database
    void exportDatabase(QString &encryption, std::function<void(bool success, QString errstr, QByteArray fileData)> cb,
                        MPDeviceProgressCb cbProgress);
    //Import database
    void importDatabase(const QByteArray &fileData, bool noDelete,
                        std::function<void(bool success, QString errstr)> cb,
                        MPDeviceProgressCb cbProgress);

    //After successfull mem mgmt mode, clients can query data
    QList<MPNode *> &getLoginNodes() { return loginNodes; }
    QList<MPNode *> &getDataNodes() { return dataNodes; }

    //true if device is a mini
    bool isMini() { return isMiniFlag; }
    //true if device fw version is at least 1.2
    bool isFw12() { return isFw12Flag; }

    QList<QVariantMap> getFilesCache();
    bool hasFilesCache();
    void getStoredFiles(std::function<void(bool, QList<QVariantMap>)> cb);
    void updateFilesCache();
    void addFileToCache(QString fileName, int size);
    void updateFileInCache(QString fileName, int size);
    void removeFileFromCache(QString fileName);
    QString getDBBackupFile();
    void setDBBackupFile(const QString &backupFile);

    /*!
     * \brief readDBBackupFile
     * \return path of the backup dile saved with QSettings
     */
    QString readDBBackupFile() const;

    /*! \brief Compares credebtials change number from Mooltipass and from backup file.
     *
     * This function should be called after both cardCPZ and
     * credentialsDbChangeNumber are set. "hashedCardCPZ" is needed to get
     * corresponding backup file that holds one of compared numbers.
     */
    void checkCredentialsDbChangeNumbers(const QString &dbBackupFile);

    void startWatchDbBackupFile();
    void stopWatchDbBackupfile();

signals:
    /* Signal emited by platform code when new data comes from MP */
    /* A signal is used for platform code that uses a dedicated thread */
    void platformDataRead(const QByteArray &data);

    /* the command has failed in platform code */
    void platformFailed();
    void filesCacheChanged();
    void hashedCardCPZChanged();
    void credentialsDiffer(int);

private slots:
    void newDataRead(const QByteArray &data);
    void commandFailed();
    void sendDataDequeue(); //execute commands from the command queue
    void runAndDequeueJobs(); //execute AsyncJobs from the jobs queues
    void onCardCPZChanged();

private:
    /* Platform function for starting a read, should be implemented in platform class */
    virtual void platformRead() {}

    /* Platform function for writing data, should be implemented in platform class */
    virtual void platformWrite(const QByteArray &data) { Q_UNUSED(data); }

    void loadLoginNode(AsyncJobs *jobs, const QByteArray &address,
                       MPDeviceProgressCb cbProgress);
    void loadLoginChildNode(AsyncJobs *jobs, MPNode *parent, MPNode *parentClone, const QByteArray &address);
    void loadDataNode(AsyncJobs *jobs, const QByteArray &address, bool load_childs,
                      MPDeviceProgressCb cbProgress);
    void loadDataChildNode(AsyncJobs *jobs, MPNode *parent, MPNode *parentClone, const QByteArray &address, MPDeviceProgressCb cbProgress, quint32 nbBytesFetched);
    void loadSingleNodeAndScan(AsyncJobs *jobs, const QByteArray &address,
                               MPDeviceProgressCb cbProgress);

    void createJobAddContext(const QString &service, AsyncJobs *jobs, bool isDataNode = false);

    bool getDataNodeCb(AsyncJobs *jobs,
                       MPDeviceProgressCb cbProgress,
                       const QByteArray &data, bool &done);
    bool setDataNodeCb(AsyncJobs *jobs, int current,
                       MPDeviceProgressCb cbProgress,
                       const QByteArray &data, bool &done);

    // Functions added by mathieu for MMM
    void memMgmtModeReadFlash(AsyncJobs *jobs, bool fullScan, MPDeviceProgressCb cbProgress, bool getCreds, bool getData, bool getDataChilds);
    MPNode *findNodeWithAddressInList(QList<MPNode *> list, const QByteArray &address, const quint32 virt_addr = 0);
    MPNode* findCredParentNodeGivenChildNodeAddr(const QByteArray &address, const quint32 virt_addr);
    void addWriteNodePacketToJob(AsyncJobs *jobs, const QByteArray &address, const QByteArray &data, std::function<void(void)> writeCallback);
    void startImportFileMerging(MPDeviceProgressCb progressCb, std::function<void(bool success, QString errstr)> cb, bool noDelete);
    void loadFreeAddresses(AsyncJobs *jobs, const QByteArray &addressFrom, bool discardFirstAddr, MPDeviceProgressCb cbProgress);
    MPNode *findNodeWithNameInList(QList<MPNode *> list, const QString& name, bool isParent);
    void deletePossibleFavorite(QByteArray parentAddr, QByteArray childAddr);
    bool finishImportFileMerging(QString &stringError, bool noDelete);
    QByteArray getNextNodeAddressInMemory(const QByteArray &address);
    quint16 getFlashPageFromAddress(const QByteArray &address);
    MPNode *findNodeWithServiceInList(const QString &service);
    MPNode *findNodeWithLoginInList(const QString &login);
    quint8 getNodeIdFromAddress(const QByteArray &address);
    QByteArray getMemoryFirstNodeAddress(void);
    quint16 getNumberOfPages(void);
    quint16 getNodesPerPage(void);
    void detagPointedNodes(void);
    bool tagFavoriteNodes(void);

    // Functions added by mathieu for MMM : checks & repairs
    bool addOrphanParentToDB(MPNode *parentNodePt, bool isDataParent, bool addPossibleChildren);
    bool checkLoadedNodes(bool checkCredentials, bool checkData, bool repairAllowed);
    bool tagPointedNodes(bool tagCredentials, bool tagData, bool repairAllowed);
    bool addOrphanParentChildsToDB(MPNode *parentNodePt, bool isDataParent);
    bool removeEmptyParentFromDB(MPNode* parentNodePt, bool isDataParent);
    bool readExportFile(const QByteArray &fileData, QString &errorString);
    bool readExportPayload(QJsonArray dataArray, QString &errorString, bool isMooltiApp);
    bool removeChildFromDB(MPNode* parentNodePt, MPNode* childNodePt, bool deleteEmptyParent);
    bool addChildToDB(MPNode* parentNodePt, MPNode* childNodePt);
    bool deleteDataParentChilds(MPNode *parentNodePt);
    MPNode* addNewServiceToDB(const QString &service);
    bool addOrphanChildToDB(MPNode* childNodePt);
    QByteArray generateExportFileData(const QString &encryption = "none");
    void cleanImportedVars(void);
    void cleanMMMVars(void);

    // checks for exported file
    inline bool isCorrectMooltiAppFile(const QJsonArray &dataArray);

    // Functions added by mathieu for unit testing
    bool testCodeAgainstCleanDBChanges(AsyncJobs *jobs);

    // Generate save packets
    bool generateSavePackets(AsyncJobs *jobs, bool tackleCreds, bool tackleData, MPDeviceProgressCb cbProgress);

    // once we fetched free addresses, this function is called
    void changeVirtualAddressesToFreeAddresses(void);

    // Crypto
    quint64 getUInt64EncryptionKey();
    QString encryptSimpleCrypt(const QByteArray &data);
    QByteArray decryptSimpleCrypt(const QString &payload);

    // Last page scanned
    quint16 lastFlashPageScanned = 0;

    void updateParam(MPParams::Param param, bool en);
    void updateParam(MPParams::Param param, int val);

    /*!
     * \brief saveDBBackupFile
     * Saves path to backup file with QSettings
     */
    void saveDBBackupFile(const QString &backupFile);

    void addFileToWatcher(const QString &backupFile);

    //timer that asks status
    QTimer *statusTimer = nullptr;

    //local vars for performance diagnostics
    qint64 diagLastSecs;
    quint32 diagNbBytesRec;
    quint32 diagLastNbBytesPSec;

    //command queue
    QQueue<MPCommand> commandQueue;

    //passwords we need to change after leaving mmm
    QList<QStringList> mmmPasswordChangeArray;

    // Number of new addresses we need
    quint32 newAddressesNeededCounter = 0;
    quint32 newAddressesReceivedCounter = 0;

    // Buffer containing the free addresses we will need
    QList<QByteArray> freeAddresses;

    // Values loaded when needed (e.g. mem mgmt mode)
    QByteArray ctrValue;
    QByteArray startNode;
    quint32 virtualStartNode = 0;
    QByteArray startDataNode;
    quint32 virtualDataStartNode = 0;
    QList<QByteArray> cpzCtrValue;
    QList<QByteArray> favoritesAddrs;
    QList<MPNode *> loginNodes;         //list of all parent nodes for credentials
    QList<MPNode *> loginChildNodes;    //list of all parent nodes for credentials
    QList<MPNode *> dataNodes;          //list of all parent nodes for data nodes
    QList<MPNode *> dataChildNodes;     //list of all parent nodes for data nodes

    // Payload to send when we need to add an unknown card
    QByteArray unknownCardAddPayload;

    // Clones of these values, used when modifying them in MMM
    quint8 credentialsDbChangeNumberClone;
    quint8 dataDbChangeNumberClone;
    QByteArray ctrValueClone;
    QByteArray startNodeClone;
    QByteArray startDataNodeClone;
    QList<QByteArray> cpzCtrValueClone;
    QList<QByteArray> favoritesAddrsClone;
    QList<MPNode *> loginNodesClone;         //list of all parent nodes for credentials
    QList<MPNode *> loginChildNodesClone;    //list of all parent nodes for credentials
    QList<MPNode *> dataNodesClone;          //list of all parent nodes for data nodes
    QList<MPNode *> dataChildNodesClone;     //list of all parent nodes for data nodes

    // Imported values
    bool isMooltiAppImportFile;
    quint32 moolticuteImportFileVersion;
    quint8 importedCredentialsDbChangeNumber;
    quint8 importedDataDbChangeNumber;
    quint32 importedDbMiniSerialNumber;
    QByteArray importedCtrValue;
    QByteArray importedStartNode;
    quint32 importedVirtualStartNode;
    QByteArray importedStartDataNode;
    quint32 importedVirtualDataStartNode;
    QList<QByteArray> importedCpzCtrValue;
    QList<QByteArray> importedFavoritesAddrs;
    QList<MPNode *> importedLoginNodes;         //list of all parent nodes for credentials
    QList<MPNode *> importedLoginChildNodes;    //list of all parent nodes for credentials
    QList<MPNode *> importedDataNodes;          //list of all parent nodes for data nodes
    QList<MPNode *> importedDataChildNodes;     //list of all parent nodes for data nodes

    bool isMiniFlag = false;            // true if fw is mini
    bool isFw12Flag = false;            // true if fw is at least v1.2

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

    //Used to maintain progression for current job
    int progressTotal;
    int progressCurrent;

    FilesCache filesCache;
    QFileSystemWatcher *m_credentialsDbFileWatcher;

    bool m_credentialsDbChangeNumberSet;
};

bool MPDevice::isCorrectMooltiAppFile(const QJsonArray &dataArray)
{
    return dataArray[9].toString() == "moolticute" && dataArray.size() == 14;
}

#endif // MPDEVICE_H
