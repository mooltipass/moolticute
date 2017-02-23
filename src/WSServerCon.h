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

class WSServerCon: public QObject
{
    Q_OBJECT
public:
    WSServerCon(QWebSocket *conn);
    virtual ~WSServerCon();

    void sendJsonMessage(const QJsonObject &data);
    void resetDevice(MPDevice *dev);
    void sendInitialStatus();

signals:
    void notifyAllClients(const QJsonObject &obj);

private slots:
    void processMessage(const QString &msg);

    void statusChanged();

    //parameters slots that sends json to websocket
    void sendKeyboardLayout();
    void sendLockTimeoutEnabled();
    void sendLockTimeout();
    void sendScreensaver();
    void sendUserRequestCancel();
    void sendUserInteractionTimeout();
    void sendFlashScreen();
    void sendOfflineMode();
    void sendTutorialEnabled();
    void sendMemMgmtMode();
    void sendVersion();
    void sendScreenBrightness();
    void sendKnockEnabled();
    void sendKnockSensitivity();
    void sendRandomStartingPin();
    void sendHashDisplayEnabled();
    void sendLockUnlockMode();
    void sendKeyAfterLoginSendEnable();
    void sendKeyAfterLoginSend();
    void sendKeyAfterPassSendEnable();
    void sendKeyAfterPassSend();
    void sendDelayAfterKeyEntryEnable();
    void sendDelayAfterKeyEntry();
    void sendDeviceUID();

private:
    QWebSocket *wsClient;

    MPDevice *mpdevice = nullptr;

    QString clientUid;

    void processParametersSet(const QJsonObject &data);
    void sendFailedJson(QJsonObject obj, QString errstr = QString());
    QString getRequestId(const QJsonValue &v);
};

#endif // WSSERVERCON_H
