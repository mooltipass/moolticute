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
#ifndef WSCLIENT_H
#define WSCLIENT_H

#include <QtCore>
#include <QWebSocket>
#include <QJsonDocument>
#include "Common.h"
#include "QtHelper.h"

class WSClient: public QObject
{
    Q_OBJECT

    QT_WRITABLE_PROPERTY(bool, connected, false)
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
    QT_WRITABLE_PROPERTY(int, screenBrightness, 0)
    QT_WRITABLE_PROPERTY(bool, knockEnabled, false)
    QT_WRITABLE_PROPERTY(int, knockSensitivity, 0)
    QT_WRITABLE_PROPERTY(bool, memMgmtMode, false)

    QT_WRITABLE_PROPERTY(Common::MPHwVersion, mpHwVersion, Common::MP_Classic)
    QT_WRITABLE_PROPERTY(QString, fwVersion, QString())
    QT_WRITABLE_PROPERTY(quint32, hwSerial, 0)
    QT_WRITABLE_PROPERTY(int, hwMemory, 0)

    QT_WRITABLE_PROPERTY(bool, keyAfterLoginSendEnable, false)
    QT_WRITABLE_PROPERTY(int, keyAfterLoginSend, 0)
    QT_WRITABLE_PROPERTY(bool, keyAfterPassSendEnable, false)
    QT_WRITABLE_PROPERTY(int, keyAfterPassSend, 0)
    QT_WRITABLE_PROPERTY(bool, delayAfterKeyEntryEnable, false)
    QT_WRITABLE_PROPERTY(int, delayAfterKeyEntry, 0)


    QT_WRITABLE_PROPERTY(bool, randomStartingPin, false)
    QT_WRITABLE_PROPERTY(bool, displayHash, false)
    QT_WRITABLE_PROPERTY(int, lockUnlockMode, false)

    QT_WRITABLE_PROPERTY(qint64, uid, -1)

    QT_WRITABLE_PROPERTY(QString, cardId, QString())
    QT_WRITABLE_PROPERTY(int, credentialsDbChangeNumber, 0)
    QT_WRITABLE_PROPERTY(int, dataDbChangeNumber, 0)

    QT_WRITABLE_PROPERTY(QString, mainMCUVersion, QString())
    QT_WRITABLE_PROPERTY(QString, auxMCUVersion, QString())

public:
    explicit WSClient(QObject *parent = nullptr);
    ~WSClient();

    void openWebsocket();
    void closeWebsocket();

    QJsonObject &getMemoryData() { return memData; }
    QJsonArray &getFilesCache() { return filesCache; }

    bool isMPMini() const;

    bool isMPBLE() const;

    bool isConnected() const;

    bool isDeviceConnected() const;

    bool requestDeviceUID(const QByteArray &key);

    void sendEnterMMRequest(bool wantData = false);
    void sendLeaveMMRequest();

    void addOrUpdateCredential(const QString &service, const QString &login,
                               const QString &password, const QString &description = {});

    void requestPassword(const QString &service, const QString &login);

    void requestDataFile(const QString &service);
    void sendDataFile(const QString &service, const QByteArray &data);
    void deleteDataFilesAndLeave(const QStringList &services);

    void requestResetCard();

    void serviceExists(bool isDatanode, const QString &service);

    void sendCredentialsMM(const QJsonArray &creds);

    void exportDbFile(const QString &encryption);
    void importDbFile(const QByteArray &fileData, bool noDelete);
    void importCSVFile(const QList<QStringList> &fileData);

    void sendListFilesCacheRequest();
    void sendRefreshFilesCacheRequest();

    void sendPlatInfoRequest();
    void sendFlashMCU(QString type);
    void sendUploadBundle(QString bundleFilePath);
    void sendFetchData(QString fileName, Common::FetchType fetchType);
    void sendStopFetchData();

    bool isFw12();

    void sendLockDevice();

signals:
    void wsConnected();
    void wsDisconnected();
    void memoryDataChanged();
    void passwordUnlocked(const QString & service, const QString & login, const QString & password, bool success);
    void credentialsUpdated(const QString & service, const QString & login, const QString & description, bool success);
    void showAppRequested();
    void progressChanged(int total, int current, QString statusMsg);
    void memcheckFinished(bool success, int freeBlocks = 0, int totalBlocks = 0);
    void dataFileRequested(const QString &service, const QByteArray &data, bool success);
    void dataFileSent(const QString &service, bool success);
    void dataFileDeleted(const QString &service, bool success);
    void credentialsExists(const QString &service, bool exists);
    void dataNodeExists(const QString &service, bool exists);
    void dbExported(const QByteArray &fileData, bool success);
    void dbImported(bool success, QString message);
    void memMgmtModeFailed(int errCode, QString errMsg);
    void filesCacheChanged(bool isInSync);
    void cardDbMetadataChanged(QString cardId, int credentialsDbChangeNumber, int dataDbChangeNumber);
    void cardResetFinished(bool successfully);
    void displayStatusWarning();
    void displayDebugPlatInfo(int auxMajor, int auxMinor, int mainMajor, int mainMinor);
    void displayUploadBundleResult(bool success);
    void deleteDataNodesFinished();

public slots:
    void sendJsonData(const QJsonObject &data);
    void queryRandomNumbers();
    void sendLoginJson(QString message, QString loginName);
    void sendDomainJson(QString message, QString serviceName);

private slots:
    void onWsConnected();
    void onWsDisconnected();
    void onWsError();
    void onTextMessageReceived(const QString &message);

private:
    void updateParameters(const QJsonObject &data);

    QWebSocket *wsocket = nullptr;

    QJsonObject memData;
    QJsonArray filesCache;

    QTimer *randomNumTimer = nullptr;
};

#endif // WSCLIENT_H
