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
}

void WSClient::onWsDisconnected()
{
    qDebug() << "Websocket disconnect";
    force_connected(false);
    closeWebsocket();

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
        if (o.contains("failed") && o["failed"].toBool())
            emit askPasswordDone(false, QString());
        else
            emit askPasswordDone(true, o["password"].toString());
    }
    else if (rootobj["msg"] == "version_changed")
    {
        QJsonObject o = rootobj["data"].toObject();
        if (o["hw_version"].toString().contains("mini"))
            set_mpHwVersion(Common::MP_Mini);
        else
            set_mpHwVersion(Common::MP_Classic);
    }
    else if (rootobj["msg"] == "set_credential")
    {
        QJsonObject o = rootobj["data"].toObject();
        if (o.contains("failed") && o["failed"].toBool())
            emit addCredentialDone(false);
        else
            emit addCredentialDone(true);
    }
    else if (rootobj["msg"] == "show_app")
    {
        emit showAppRequested();
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
        set_lockTimeout(data["value"].toInt() / 60);
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
    else if (param == "key_after_login_enabled")
        set_keyAfterLoginSendEnable(data["value"].toBool());
    else if (param == "key_after_login")
        set_keyAfterLoginSend(data["value"].toInt());
    else if (param == "key_after_pass_enabled")
        set_keyAfterPassSendEnable(data["value"].toBool());
    else if (param == "key_after_pass")
        set_keyAfterPassSend(data["value"].toInt());
    else if (param == "delay_after_key_enable")
        set_delayAfterKeyEntryEnable(data["value"].toBool());
    else if (param == "delay_after_key")
        set_delayAfterKeyEntry(data["value"].toInt());
}
