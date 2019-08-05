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
#include "DeviceSettings.h"
#include "MPSettingsMini.h"

using MPCommandCb = std::function<void(bool success, const QByteArray &data, bool &done)>;
using MPDeviceProgressCb = std::function<void(const QVariantMap &data)>;
/* Example usage of the above function
 * MPDeviceProgressCb cb;
 * QVariantMap progressData = { {"total", 100},
 *                      {"current", 2},
 *                      {"msg", "message with %1 arguments: %2 %3"},
 *                      {"msg_args", QVariantList({1, "23", "23423"})}};
 * cb(data)
*/
using MessageHandlerCb = std::function<void(bool success, QString errstr)>;
using MessageHandlerCbData = std::function<void(bool success, QString errstr, QByteArray data)>;

class MPDeviceBleImpl;
class IMessageProtocol;

class MPCommand
{
public:
    QVector<QByteArray> data;
    MPCommandCb cb;
    bool running = false;

    QTimer *timerTimeout = nullptr;
    int retry = CMD_MAX_RETRY;
    int retries_done = 0;
    qint64 sent_ts = 0;

    bool checkReturn = true;

    // For BLE
    QByteArray response;
    int responseSize = 0;
};

class MPDevice: public QObject
{
    Q_OBJECT

    QT_WRITABLE_PROPERTY(Common::MPStatus, status, Common::UnknownStatus)
    QT_WRITABLE_PROPERTY(bool, memMgmtMode, false)
    QT_WRITABLE_PROPERTY(QString, hwVersion, QString())
    QT_WRITABLE_PROPERTY(quint32, serialNumber, 0)              // serial number if firmware is above 1.2
    QT_WRITABLE_PROPERTY(int, flashMbSize, 0)

    QT_WRITABLE_PROPERTY(quint32, credentialsDbChangeNumber, 0)  // credentials db change number
    QT_WRITABLE_PROPERTY(quint32, dataDbChangeNumber, 0)         // data db change number
    QT_WRITABLE_PROPERTY(QByteArray, cardCPZ, QByteArray())     // Card CPZ

    QT_WRITABLE_PROPERTY(qint64, uid, -1)

public:
    MPDevice(QObject *parent);
    virtual ~MPDevice();

    friend class MPDeviceBleImpl;
    friend class MPSettingsMini;

    void setupMessageProtocol();
    void sendInitMessages();
    /* Send a command with data to the device */
    void sendData(MPCmd::Command cmd, const QByteArray &data = QByteArray(), quint32 timeout = CMD_DEFAULT_TIMEOUT, MPCommandCb cb = [](bool, const QByteArray &, bool &){}, bool checkReturn = true);
    void sendData(MPCmd::Command cmd, quint32 timeout, MPCommandCb cb);
    void sendData(MPCmd::Command cmd, MPCommandCb cb);
    void sendData(MPCmd::Command cmd, const QByteArray &data, MPCommandCb cb);
    void enqueueAndRunJob(AsyncJobs* jobs);

    void getUID(const QByteArray & key);

    //mem mgmt mode
    //cbFailure is used to propagate an error to clients when entering mmm
    void startMemMgmtMode(bool wantData,
                          const MPDeviceProgressCb &cbProgress,
                          const std::function<void(bool success, int errCode, QString errMsg)> &cb);
    void exitMemMgmtMode(bool setMMMBool = true);
    void startIntegrityCheck(const std::function<void(bool success, int freeBlocks, int totalBlocks, QString errstr)> &cb,
                             const MPDeviceProgressCb &cbProgress);

    //Send current date to MP
    void setCurrentDate();

    //Get database change numbers
    void getChangeNumbers();

    //Get current card CPZ
    void getCurrentCardCPZ();

    //Ask a password for specified service/login to MP
    void getCredential(QString service, const QString &login, const QString &fallback_service, const QString &reqid,
                       std::function<void(bool success, QString errstr, const QString &_service, const QString &login, const QString &pass, const QString &desc)> cb);

    //Add or Set service/login/pass/desc in MP
    void setCredential(QString service, const QString &login,
                       const QString &pass, const QString &description, bool setDesc,
                       MessageHandlerCb cb);

    //Delete credential in MMM and leave
    void delCredentialAndLeave(QString service, const QString &login,
                               const MPDeviceProgressCb &cbProgress,
                               MessageHandlerCb cb);

    //get 32 random bytes from device
    void getRandomNumber(std::function<void(bool success, QString errstr, const QByteArray &nums)> cb);

    //Send a cancel request to device
    void cancelUserRequest(const QString &reqid);
    void writeCancelRequest();

    //Request for a raw data node from the device
    void getDataNode(QString service, const QString &fallback_service, const QString &reqid,
                     std::function<void(bool success, QString errstr, QString service, QByteArray rawData)> cb,
                     const MPDeviceProgressCb &cbProgress);

    //Set data to a context on the device
    void setDataNode(QString service, const QByteArray &nodeData,
                     MessageHandlerCb cb,
                     const MPDeviceProgressCb &cbProgress);

    //Delete a data context from the device
    void deleteDataNodesAndLeave(QStringList services,
                                 MessageHandlerCb cb,
                                 const MPDeviceProgressCb &cbProgress);

    //Check is credential/data node exists
    void serviceExists(bool isDatanode, QString service, const QString &reqid,
                       std::function<void(bool success, QString errstr, QString service, bool exists)> cb);

    // Import unencrypted credentials from CSV
    void importFromCSV(const QJsonArray &creds, const MPDeviceProgressCb &cbProgress,
                          MessageHandlerCb cb);

    //Set full list of credentials in MMM
    void setMMCredentials(const QJsonArray &creds, bool noDelete, const MPDeviceProgressCb &cbProgress,
                          MessageHandlerCb cb);

    //Export database
    void exportDatabase(const QString &encryption, std::function<void(bool success, QString errstr, QByteArray fileData)> cb,
                        const MPDeviceProgressCb &cbProgress);
    //Import database
    void importDatabase(const QByteArray &fileData, bool noDelete,
                        MessageHandlerCb cb,
                        const MPDeviceProgressCb &cbProgress);

    // Reset smart card
    void resetSmartCard(MessageHandlerCb cb);

    // Lock the devide.
    void lockDevice(const MessageHandlerCb &cb);

    void getAvailableUsers(const MessageHandlerCb &cb);

    // Returns bleImpl
    MPDeviceBleImpl* ble() const;
    DeviceSettings* settings() const;

    IMessageProtocol* getMesProt() const;

    //After successfull mem mgmt mode, clients can query data
    QList<MPNode *> &getLoginNodes() { return loginNodes; }
    QList<MPNode *> &getDataNodes() { return dataNodes; }

    //true if device is a mini
    inline bool isMini() const { return DeviceType::MINI == deviceType; }
    //true if device is a ble
    inline bool isBLE() const { return DeviceType::BLE == deviceType;}
    //true if device fw version is at least 1.2
    inline bool isFw12() const { return isFw12Flag; }

    QList<QVariantMap> getFilesCache();
    bool hasFilesCache();
    bool isFilesCacheInSync() const;
    void getStoredFiles(std::function<void(bool, QList<QVariantMap>)> cb);
    void updateFilesCache();
    void addFileToCache(QString fileName, int size);
    void updateFileInCache(QString fileName, int size);
    void removeFileFromCache(QString fileName);

signals:
    /* Signal emited by platform code when new data comes from MP */
    /* A signal is used for platform code that uses a dedicated thread */
    void platformDataRead(const QByteArray &data);

    /* the command has failed in platform code */
    void platformFailed();
    void filesCacheChanged();
    void dbChangeNumbersChanged(const int credentialsDbChangeNumber, const int dataDbChangeNumber);

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

    void loadLoginNode(AsyncJobs *jobs, const QByteArray &address,
                       const MPDeviceProgressCb &cbProgress);
    void loadLoginChildNode(AsyncJobs *jobs, MPNode *parent, MPNode *parentClone, const QByteArray &address);
    void loadDataNode(AsyncJobs *jobs, const QByteArray &address, bool load_childs,
                      const MPDeviceProgressCb &cbProgress);
    void loadDataChildNode(AsyncJobs *jobs, MPNode *parent, MPNode *parentClone, const QByteArray &address, const MPDeviceProgressCb &cbProgress, quint32 nbBytesFetched);
    void loadSingleNodeAndScan(AsyncJobs *jobs, const QByteArray &address,
                               const MPDeviceProgressCb &cbProgress);

    void createJobAddContext(const QString &service, AsyncJobs *jobs, bool isDataNode = false);

    bool getDataNodeCb(AsyncJobs *jobs,
                       const MPDeviceProgressCb &cbProgress,
                       const QByteArray &data, bool &done);
    bool setDataNodeCb(AsyncJobs *jobs, int current,
                       const MPDeviceProgressCb &cbProgress,
                       const QByteArray &data, bool &done);

    // Functions added by mathieu for MMM
    void memMgmtModeReadFlash(AsyncJobs *jobs, bool fullScan, const MPDeviceProgressCb &cbProgress, bool getCreds, bool getData, bool getDataChilds);
    MPNode *findNodeWithAddressInList(QList<MPNode *> list, const QByteArray &address, const quint32 virt_addr = 0);
    MPNode* findCredParentNodeGivenChildNodeAddr(const QByteArray &address, const quint32 virt_addr);
    void addWriteNodePacketToJob(AsyncJobs *jobs, const QByteArray &address, const QByteArray &data, std::function<void(void)> writeCallback);
    void startImportFileMerging(const MPDeviceProgressCb &progressCb, MessageHandlerCb cb, bool noDelete);
    void loadFreeAddresses(AsyncJobs *jobs, const QByteArray &addressFrom, bool discardFirstAddr, const MPDeviceProgressCb &cbProgress);
    MPNode *findNodeWithAddressWithGivenParentInList(QList<MPNode *> list,  MPNode *parent, const QByteArray &address, const quint32 virt_addr);
    MPNode *findNodeWithLoginWithGivenParentInList(QList<MPNode *> list,  MPNode *parent, const QString& name);
    MPNode *findNodeWithNameInList(QList<MPNode *> list, const QString& name, bool isParent);
    void deletePossibleFavorite(QByteArray parentAddr, QByteArray childAddr);
    bool finishImportFileMerging(QString &stringError, bool noDelete);
    QByteArray getNextNodeAddressInMemory(const QByteArray &address);
    quint16 getFlashPageFromAddress(const QByteArray &address);
    MPNode *findNodeWithServiceInList(const QString &service);
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
    bool readExportPayload(QJsonArray dataArray, QString &errorString);
    bool removeChildFromDB(MPNode* parentNodePt, MPNode* childNodePt, bool deleteEmptyParent, bool deleteFromList);
    bool addChildToDB(MPNode* parentNodePt, MPNode* childNodePt);
    bool deleteDataParentChilds(MPNode *parentNodePt);
    MPNode* addNewServiceToDB(const QString &service);
    bool addOrphanChildToDB(MPNode* childNodePt);
    QByteArray generateExportFileData(const QString &encryption = "none");
    void cleanImportedVars(void);
    void cleanMMMVars(void);

    // Functions added by mathieu for unit testing
    bool testCodeAgainstCleanDBChanges(AsyncJobs *jobs);

    // Generate save packets
    bool generateSavePackets(AsyncJobs *jobs, bool tackleCreds, bool tackleData, const MPDeviceProgressCb &cbProgress);

    // once we fetched free addresses, this function is called
    void changeVirtualAddressesToFreeAddresses(void);

    // Crypto
    quint64 getUInt64EncryptionKey();
    QString encryptSimpleCrypt(const QByteArray &data);
    QByteArray decryptSimpleCrypt(const QString &payload);

    // Last page scanned
    quint16 lastFlashPageScanned = 0;

    //timer that asks status
    QTimer *statusTimer = nullptr;

    //local vars for performance diagnostics
    qint64 diagLastSecs;
    quint32 diagNbBytesRec;
    quint32 diagLastNbBytesPSec;
    int diagFreeBlocks = 0;
    int diagTotalBlocks = 0;

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
    quint32 credentialsDbChangeNumberClone;
    quint32 dataDbChangeNumberClone;
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

    bool isFw12Flag = false;            // true if fw is at least v1.2

    //this queue is used to put jobs list in a wait
    //queue. it prevents other clients to query something
    //when an AsyncJobs is currently running.
    //An AsyncJobs can also be removed if it was not started (using cancelUserRequest for example)
    //All AsyncJobs does have an <id>
    QQueue<AsyncJobs *> jobsQueue;
    AsyncJobs *currentJobs = nullptr;

    //this is a cache for data upload
    QByteArray currentDataNode;

    //Used to maintain progression for current job
    int progressTotal;
    int progressCurrent;

    FilesCache filesCache;

    bool m_isDebugMsg = false;
    //Message Protocol
    IMessageProtocol *pMesProt = nullptr;
    MPDeviceBleImpl *bleImpl = nullptr;
    DeviceSettings *pSettings = nullptr;

protected:
    DeviceType deviceType = DeviceType::MOOLTIPASS;
};

#endif // MPDEVICE_H
