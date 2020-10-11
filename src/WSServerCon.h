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
#ifndef WSSERVERCON_H
#define WSSERVERCON_H

#include <QtCore>
#include <QWebSocket>
#include "Common.h"
#include "MPManager.h"

class WSServer;
class HaveIBeenPwned;

class WSServerCon: public QObject
{
    Q_OBJECT
public:
    WSServerCon(QWebSocket *conn);
    virtual ~WSServerCon();

    void sendJsonMessage(const QJsonObject &data);
    void sendJsonMessageString(const QString &data);
    void resetDevice(MPDevice *dev);
    void sendInitialStatus();

    QString getClientUid() { return clientUid; }

signals:
    void notifyAllClients(const QJsonObject &obj);
    void sendMessageToGUI(const QString &msg, bool &isGuiRunning);
    void parameterProcessFinished();

private slots:
    void processMessage(const QString &msg);

    void statusChanged();

    //parameters slots that sends json to websocket
    void sendParams(int value, int param);
    void sendParams(bool value, int param);
    void sendMemMgmtMode();
    void sendVersion();
    void sendDeviceUID();
    void sendFilesCache();

    void sendDeviceLanguage(const QJsonObject& langs);
    void sendKeyboardLayout(const QJsonObject& layouts);

    void sendCardDbMetadata();

    void sendHibpNotification(QString credInfo, QString pwnedNum);
    void sendUserSettings(QJsonObject settings);

    void sendBatteryPercent(int batteryPct);
private:
    bool checkMemModeEnabled(const QJsonObject &root);
    bool processSetCredential(QJsonObject &root, QJsonObject &o);

    QWebSocket *wsClient;

    MPDevice *mpdevice = nullptr;

    QString clientUid;

    HaveIBeenPwned *hibp = nullptr;

    void processParametersSet(const QJsonObject &data);
    void sendFailedJson(QJsonObject obj, QString errstr = QString(), int errCode = -999);
    QString getRequestId(const QJsonValue &v);
    void checkHaveIBeenPwned(const QString &service, const QString &login, const QString &password);
    void processMessageMini(QJsonObject root, const MPDeviceProgressCb &cbProgress);
    void processMessageBLE(QJsonObject root, const MPDeviceProgressCb &cbProgress);
};

#endif // WSSERVERCON_H
