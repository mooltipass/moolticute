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

public:
    explicit WSClient(QObject *parent = nullptr);
    ~WSClient();

    void closeWebsocket();

    QJsonObject &getMemoryData() { return memData; }

    bool isMPMini() const;

    bool isConnected() const;

    bool requestDeviceUID(const QByteArray &key);

    void sendEnterMMRequest();
    void sendLeaveMMRequest();

    void addOrUpdateCredential(const QString &service, const QString &login,
                       const QString &password, const QString &description = {});

    void requestPassword(const QString &service, const QString &login);

    void requestDataFile(const QString &service);
    void sendDataFile(const QString &service, const QByteArray &data);

    void serviceExists(bool isDatanode, const QString &service);

    void sendCredentialsMM(const QJsonArray &creds);

signals:
    void wsConnected();
    void wsDisconnected();
    void memoryDataChanged();
    void passwordUnlocked(const QString & service, const QString & login, const QString & password, bool success);
    void credentialsUpdated(const QString & service, const QString & login, const QString & description, bool success);
    void showAppRequested();
    void progressChanged(int total, int current);
    void memcheckFinished(bool success);
    void dataFileRequested(const QString &service, const QByteArray &data, bool success);
    void dataFileSent(const QString &service, bool success);
    void credentialsExists(const QString &service, bool exists);
    void dataNodeExists(const QString &service, bool exists);

public slots:
    void sendJsonData(const QJsonObject &data);

private slots:
    void onWsConnected();
    void onWsDisconnected();
    void onWsError();
    void onTextMessageReceived(const QString &message);

private:
    void openWebsocket();

    void udateParameters(const QJsonObject &data);

    QWebSocket *wsocket = nullptr;

    QJsonObject memData;
};

#endif // WSCLIENT_H
