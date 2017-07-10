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
#include "WSClient.h"

#define WS_URI      "ws://localhost"

WSClient::WSClient(QObject *parent):
    QObject(parent)
{
    openWebsocket();
}

WSClient::~WSClient()
{
    if (wsocket)
    {
        if (wsocket->state() == QAbstractSocket::ConnectedState)
            wsocket->close();
        delete wsocket;
        wsocket = nullptr;
    }
}

void WSClient::openWebsocket()
{
    if (!wsocket)
    {
        wsocket = new QWebSocket();
        connect(wsocket, &QWebSocket::connected, this, &WSClient::onWsConnected);
        connect(wsocket, &QWebSocket::disconnected, this, &WSClient::onWsDisconnected);
        connect(wsocket, SIGNAL(error(QAbstractSocket::SocketError)),
                this, SLOT(onWsError()));
    }

    QString url = QString("%1:%2").arg(WS_URI).arg(MOOLTICUTE_DAEMON_PORT);
    wsocket->open(url);
}

void WSClient::closeWebsocket()
{
    if (wsocket)
    {
        wsocket->deleteLater();
        wsocket = nullptr;
    }
}

void WSClient::sendJsonData(const QJsonObject &data)
{
    QJsonDocument jdoc(data);
    //    qDebug().noquote() << jdoc.toJson();
    wsocket->sendTextMessage(jdoc.toJson());
}

void WSClient::onWsConnected()
{
    qDebug() << "Websocket connected";
    connect(wsocket, &QWebSocket::textMessageReceived, this, &WSClient::onTextMessageReceived);
    Q_EMIT wsConnected();
}

bool WSClient::isConnected() const {
    return wsocket && wsocket->state() == QAbstractSocket::ConnectedState;
}

void WSClient::onWsDisconnected()
{
    qDebug() << "Websocket disconnect";

    set_memMgmtMode(false);
    force_connected(false);
    closeWebsocket();

    Q_EMIT wsDisconnected();

    //Auto reconnect websocket connection on failure
    QTimer::singleShot(500, [=]()
    {
        qDebug() << "Websocket open()";
        openWebsocket();
    });
}

void WSClient::onWsError()
{
    qDebug() << "Websocket error: " << (wsocket?wsocket->errorString():"No websocket object");
    closeWebsocket();

    //Auto reconnect websocket connection on failure
    QTimer::singleShot(500, [=]()
    {
        qDebug() << "Websocket open()";
        openWebsocket();
    });
}

void WSClient::onTextMessageReceived(const QString &message)
{
    QJsonParseError err;
    QJsonDocument jdoc = QJsonDocument::fromJson(message.toUtf8(), &err);

    if (err.error != QJsonParseError::NoError)
    {
        qWarning() << "JSON parse error " << err.errorString();
        return;
    }
    QJsonObject rootobj = jdoc.object();

    qDebug().noquote() << "New message: " << rootobj;

    if (rootobj["msg"] == "mp_connected")
    {
        set_connected(true);
    }
    else if (rootobj["msg"] == "mp_disconnected")
    {
        set_memMgmtMode(false);
        set_connected(false);
    }
    else if (rootobj["msg"] == "status_changed")
    {
        QString st = rootobj["data"].toString();
        set_status(Common::statusFromString(st));
    }
    else if (rootobj["msg"] == "param_changed")
    {
        udateParameters(rootobj["data"].toObject());
    }
    else if (rootobj["msg"] == "memorymgmt_changed")
    {
        force_memMgmtMode(rootobj["data"].toBool());

        memData = QJsonObject();
        emit memoryDataChanged();
    }
    else if (rootobj["msg"] == "memorymgmt_data")
    {
        memData = rootobj["data"].toObject();
        emit memoryDataChanged();
    }
    else if (rootobj["msg"] == "ask_password")
    {
        QJsonObject o = rootobj["data"].toObject();
        bool success = !o.contains("failed") || !o.value("failed").toBool();
        emit passwordUnlocked(o["service"].toString(), o["login"].toString(), o["password"].toString(), success);
    }
    else if (rootobj["msg"] == "version_changed")
    {
        QJsonObject o = rootobj["data"].toObject();
        if (o["hw_version"].toString().contains("mini"))
            set_mpHwVersion(Common::MP_Mini);
        else
            set_mpHwVersion(Common::MP_Classic);
        set_fwVersion(o["hw_version"].toString());
        set_hwSerial(o["hw_serial"].toInt());
        set_hwMemory(o["flash_size"].toInt());
    }
    else if (rootobj["msg"] == "set_credential")
    {
        QJsonObject o = rootobj["data"].toObject();
        bool success = !o.contains("failed") || !o["failed"].toBool();
        credentialsUpdated(o["service"].toString(), o["login"].toString(), o["description"].toString(), success);
    }
    else if (rootobj["msg"] == "show_app")
    {
        emit showAppRequested();
    }
    else if (rootobj["msg"] == "memcheck")
    {
        QJsonObject o = rootobj["data"].toObject();
        if (o.value("failed").toBool())
            emit memcheckFinished(false);
        else
            emit memcheckFinished(true);
    }
    else if (rootobj["msg"] == "progress")
    {
        QJsonObject o = rootobj["data"].toObject();
        emit progressChanged(o["progress_total"].toInt(), o["progress_current"].toInt());
    }
    else if (rootobj["msg"] == "device_uid")
    {
        QJsonObject o = rootobj["data"].toObject();
        set_uid((qint64)o["uid"].toDouble());
    }
    else if (rootobj["msg"] == "get_data_node")
    {
        QJsonObject o = rootobj["data"].toObject();
        bool success = !o.contains("failed") || !o.value("failed").toBool();
        QByteArray b = QByteArray::fromBase64(o["node_data"].toString().toLocal8Bit());
        emit dataFileRequested(o["service"].toString(), b, success);
    }
    else if (rootobj["msg"] == "set_data_node")
    {
        QJsonObject o = rootobj["data"].toObject();
        bool success = !o.contains("failed") || !o.value("failed").toBool();
        emit dataFileSent(o["service"].toString(), success);
    }
    else if (rootobj["msg"] == "data_node_exists")
    {
        QJsonObject o = rootobj["data"].toObject();
        emit dataNodeExists(o["service"].toString(), o["exists"].toBool());
    }
    else if (rootobj["msg"] == "credential_exists")
    {
        QJsonObject o = rootobj["data"].toObject();
        emit credentialsExists(o["service"].toString(), o["exists"].toBool());
    }
}

void WSClient::udateParameters(const QJsonObject &data)
{
    QString param = data["parameter"].toString();

    if (param == "keyboard_layout")
        set_keyboardLayout(data["value"].toInt());
    else if (param == "lock_timeout_enabled")
        set_lockTimeoutEnabled(data["value"].toBool());
    else if (param == "lock_timeout")
        set_lockTimeout(data["value"].toInt());
    else if (param == "screensaver")
        set_screensaver(data["value"].toBool());
    else if (param == "user_request_cancel")
        set_userRequestCancel(data["value"].toBool());
    else if (param == "user_interaction_timeout")
        set_userInteractionTimeout(data["value"].toInt());
    else if (param == "flash_screen")
        set_flashScreen(data["value"].toBool());
    else if (param == "offline_mode")
        set_offlineMode(data["value"].toBool());
    else if (param == "tutorial_enabled")
        set_tutorialEnabled(data["value"].toBool());
    else if (param == "screen_brightness")
        set_screenBrightness(data["value"].toInt());
    else if (param == "knock_enabled")
        set_knockEnabled(data["value"].toBool());
    else if (param == "knock_sensitivity")
        set_knockSensitivity(data["value"].toInt());
    else if (param == "random_starting_pin")
        set_randomStartingPin(data["value"].toBool());
    else if (param == "hash_display")
        set_displayHash(data["value"].toBool());
    else if (param == "lock_unlock_mode")
        set_lockUnlockMode(data["value"].toInt());
    else if (param == "key_after_login_enabled")
        set_keyAfterLoginSendEnable(data["value"].toBool());
    else if (param == "key_after_login")
        set_keyAfterLoginSend(data["value"].toInt());
    else if (param == "key_after_pass_enabled")
        set_keyAfterPassSendEnable(data["value"].toBool());
    else if (param == "key_after_pass")
        set_keyAfterPassSend(data["value"].toInt());
    else if (param == "delay_after_key_enabled")
        set_delayAfterKeyEntryEnable(data["value"].toBool());
    else if (param == "delay_after_key")
        set_delayAfterKeyEntry(data["value"].toInt());
}


bool WSClient::isMPMini() const
{
    return  get_mpHwVersion() == Common::MP_Mini;
}


bool WSClient::requestDeviceUID(const QByteArray & key)
{
    m_uid = -1;
    if(!isConnected())
        return false;
    sendJsonData({{ "msg", "request_device_uid" },
                  { "data", QJsonObject{ {"key", QString::fromUtf8(key) } } }
                 });
    return true;
}


void WSClient::sendEnterMMRequest()
{
    sendJsonData({{ "msg", "start_memorymgmt" }});
}

void WSClient::sendLeaveMMRequest()
{
    sendJsonData({{ "msg", "exit_memorymgmt" }});
}

void WSClient::addOrUpdateCredential(const QString & service, const QString & login,
                                     const QString & password, const QString & description)
{
    QJsonObject o = {{ "service", service},
                     { "login",   login},
                     { "password", password },
                     { "description", description}};
    sendJsonData({{ "msg", "set_credential" },
                  { "data", o }});
}

void WSClient::requestPassword(const QString & service, const QString & login)
{

    QJsonObject d = {{ "service", service },
                     { "login", login }};
    sendJsonData({{ "msg", "ask_password" },
                  { "data", d }});
}

void WSClient::requestDataFile(const QString &service)
{
    QJsonObject d = {{ "service", service }};
    sendJsonData({{ "msg", "get_data_node" },
                  { "data", d }});
}

void WSClient::sendDataFile(const QString &service, const QByteArray &data)
{
    QJsonObject d = {{ "service", service },
                     { "node_data", QString(data.toBase64()) }};
    sendJsonData({{ "msg", "set_data_node" },
                  { "data", d }});
}

void WSClient::serviceExists(bool isDatanode, const QString &service)
{
    QJsonObject d = {{ "service", service }};
    sendJsonData({{ "msg", isDatanode? "data_node_exists": "credential_exists" },
                  { "data", d }});
}

void WSClient::sendCredentialsMM(const QJsonArray &creds)
{
    sendJsonData({{ "msg", "set_credentials" },
                  { "data", creds }});
}
