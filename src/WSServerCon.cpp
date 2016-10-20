#include "WSServerCon.h"
#include "WSServer.h"

WSServerCon::WSServerCon(QWebSocket *conn):
    wsClient(conn),
    clientUid(Common::createUid(QStringLiteral("ws-")))
{
    connect(wsClient, &QWebSocket::textMessageReceived, this, &WSServerCon::processMessage);
}

WSServerCon::~WSServerCon()
{
    delete wsClient;
}

void WSServerCon::sendJsonMessage(const QJsonObject &data)
{
    QJsonDocument jdoc(data);
    wsClient->sendTextMessage(jdoc.toJson(QJsonDocument::JsonFormat::Compact));
}

void WSServerCon::processMessage(const QString &message)
{
    QJsonParseError err;
    QJsonDocument jdoc = QJsonDocument::fromJson(message.toUtf8(), &err);

    qDebug().noquote() << "JSON API recv:" << message;

    if (err.error != QJsonParseError::NoError)
    {
        qWarning() << "JSON parse error " << err.errorString();
        return;
    }

    QJsonObject root = jdoc.object();

    if (root["msg"] == "param_set")
    {
        processParametersSet(root["data"].toObject());
    }
    else if (root["msg"] == "start_memorymgmt")
    {
        //send command to start MMM
        mpdevice->startMemMgmtMode();
    }
    else if (root["msg"] == "exit_memorymgmt")
    {
        //send command to exit MMM
        mpdevice->exitMemMgmtMode();
    }
    else if (root["msg"] == "ask_password" ||
             root["msg"] == "get_credential")
    {
        QJsonObject o = root["data"].toObject();

        QString reqid;
        if (o.contains("request_id"))
            reqid = QStringLiteral("%1-%2").arg(clientUid).arg(getRequestId(o["request_id"]));

        mpdevice->askPassword(o["service"].toString(), o["login"].toString(), o["fallback_service"].toString(),
                reqid,
                [=](bool success, QString errstr, const QString &service, const QString &login, const QString &pass)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["service"] = service;
            ores["login"] = login;
            ores["password"] = pass;
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "set_credential")
    {
        QJsonObject o = root["data"].toObject();
        mpdevice->setCredential(o["service"].toString(), o["login"].toString(),
                o["password"].toString(), o["description"].toString(),
                [=](bool success, QString errstr)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores = o;
            QJsonObject oroot = root;
            ores["password"] = "******";
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "get_random_numbers")
    {
        mpdevice->getRandomNumber([=](bool success, QString errstr, const QByteArray &rndNums)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject oroot = root;
            QJsonArray arr;
            for (int i = 0;i < rndNums.size();i++)
                arr.append((quint8)rndNums.at(i));
            oroot["data"] = arr;
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "cancel_request")
    {
        QJsonObject o = root["data"].toObject();
        QString reqid;
        if (o.contains("request_id"))
            reqid = QStringLiteral("%1-%2").arg(clientUid).arg(getRequestId(o["request_id"]));

        mpdevice->cancelUserRequest(reqid);
    }
    else if (root["msg"] == "get_data_node")
    {
        QJsonObject o = root["data"].toObject();

        QString reqid;
        if (o.contains("request_id"))
            reqid = QStringLiteral("%1-%2").arg(clientUid).arg(getRequestId(o["request_id"]));

        mpdevice->getDataNode(o["service"].toString(), o["fallback_service"].toString(),
                reqid,
                [=](bool success, QString errstr, const QString &service, const QByteArray &dataNode)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["service"] = service;
            ores["node_data"] = QString(dataNode.toBase64());
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
    else if (root["msg"] == "set_data_node")
    {
        QJsonObject o = root["data"].toObject();

        QString reqid;
        if (o.contains("request_id"))
            reqid = QStringLiteral("%1-%2").arg(clientUid).arg(getRequestId(o["request_id"]));

        QByteArray data = QByteArray::fromBase64(o["node_data"].toString().toLocal8Bit());
        if (data.isEmpty())
        {
            sendFailedJson(root, "node_data is empty");
            return;
        }

        mpdevice->setDataNode(o["service"].toString(), data,
                reqid,
                [=](bool success, QString errstr)
        {
            if (!WSServer::Instance()->checkClientExists(this))
                return;

            if (!success)
            {
                sendFailedJson(root, errstr);
                return;
            }

            QJsonObject ores;
            QJsonObject oroot = root;
            ores["node_data"] = "********";
            oroot["data"] = ores;
            sendJsonMessage(oroot);
        });
    }
}

void WSServerCon::sendFailedJson(QJsonObject obj, QString errstr)
{
    QJsonObject odata;
    odata["failed"] = true;
    if (!errstr.isEmpty())
        odata["error_message"] = errstr;
    obj["data"] = odata;
    sendJsonMessage(obj);
}

void WSServerCon::resetDevice(MPDevice *dev)
{
    mpdevice = dev;

    if (!mpdevice)
    {
        sendJsonMessage({{ "msg", "mp_disconnected" }});
        return;
    }

    sendJsonMessage({{ "msg", "mp_connected" }});

    //Whenever mp status changes, send state update to client
    connect(mpdevice, &MPDevice::statusChanged, this, &WSServerCon::statusChanged);

    connect(mpdevice, SIGNAL(keyboardLayoutChanged(int)), this, SLOT(sendKeyboardLayout()));
    connect(mpdevice, SIGNAL(lockTimeoutEnabledChanged(bool)), this, SLOT(sendLockTimeoutEnabled()));
    connect(mpdevice, SIGNAL(lockTimeoutChanged(int)), this, SLOT(sendLockTimeout()));
    connect(mpdevice, SIGNAL(screensaverChanged(bool)), this, SLOT(sendScreensaver()));
    connect(mpdevice, SIGNAL(userRequestCancelChanged(bool)), this, SLOT(sendUserRequestCancel()));
    connect(mpdevice, SIGNAL(userInteractionTimeoutChanged(int)), this, SLOT(sendUserInteractionTimeout()));
    connect(mpdevice, SIGNAL(flashScreenChanged(bool)), this, SLOT(sendFlashScreen()));
    connect(mpdevice, SIGNAL(offlineModeChanged(bool)), this, SLOT(sendOfflineMode()));
    connect(mpdevice, SIGNAL(tutorialEnabledChanged(bool)), this, SLOT(sendTutorialEnabled()));
    connect(mpdevice, SIGNAL(memMgmtModeChanged(bool)), this, SLOT(sendMemMgmtMode()));
    connect(mpdevice, SIGNAL(flashMbSizeChanged(int)), this, SLOT(sendVersion()));
    connect(mpdevice, SIGNAL(hwVersionChanged(QString)), this, SLOT(sendVersion()));
    connect(mpdevice, SIGNAL(screenBrightnessChanged(int)), this, SLOT(sendScreenBrightness()));
    connect(mpdevice, SIGNAL(knockEnabledChanged(bool)), this, SLOT(sendKnockEnabled()));
    connect(mpdevice, SIGNAL(knockSensitivityChanged(int)), this, SLOT(sendKnockSensitivity()));
}

void WSServerCon::statusChanged()
{
    qDebug() << "Update client status changed: " << this;
    sendJsonMessage({{ "msg", "status_changed" },
                     { "data", Common::MPStatusString[mpdevice->get_status()] }});
}

void WSServerCon::sendInitialStatus()
{
    //Sends initial status to any new connected client
    //is any mp connected? and if true send mp state too

    if (!mpdevice)
        sendJsonMessage({{ "msg", "mp_disconnected" }});
    else
    {
        sendJsonMessage({{ "msg", "mp_connected" }});
        sendJsonMessage({{ "msg", "status_changed" },
                         { "data", Common::MPStatusString[mpdevice->get_status()] }});
        sendKeyboardLayout();
        sendLockTimeoutEnabled();
        sendLockTimeout();
        sendScreensaver();
        sendUserRequestCancel();
        sendUserInteractionTimeout();
        sendFlashScreen();
        sendOfflineMode();
        sendTutorialEnabled();
        sendMemMgmtMode();
        sendVersion();
        sendScreenBrightness();
        sendKnockEnabled();
        sendKnockSensitivity();
    }
}

void WSServerCon::sendKeyboardLayout()
{
    QJsonObject data = {{ "parameter", "keyboard_layout" },
                        { "value", mpdevice->get_keyboardLayout() }};
    sendJsonMessage({{ "msg", "param_changed" }, { "data", data }});
}

void WSServerCon::sendLockTimeoutEnabled()
{
    QJsonObject data = {{ "parameter", "lock_timeout_enabled" },
                        { "value", mpdevice->get_lockTimeoutEnabled() }};
    sendJsonMessage({{ "msg", "param_changed" }, { "data", data }});
}

void WSServerCon::sendLockTimeout()
{
    QJsonObject data = {{ "parameter", "lock_timeout" },
                        { "value", mpdevice->get_lockTimeout() }};
    sendJsonMessage({{ "msg", "param_changed" }, { "data", data }});
}

void WSServerCon::sendScreensaver()
{
    QJsonObject data = {{ "parameter", "screensaver" },
                        { "value", mpdevice->get_screensaver() }};
    sendJsonMessage({{ "msg", "param_changed" }, { "data", data }});
}

void WSServerCon::sendUserRequestCancel()
{
    QJsonObject data = {{ "parameter", "user_request_cancel" },
                        { "value", mpdevice->get_userRequestCancel() }};
    sendJsonMessage({{ "msg", "param_changed" }, { "data", data }});
}

void WSServerCon::sendUserInteractionTimeout()
{
    QJsonObject data = {{ "parameter", "user_interaction_timeout" },
                        { "value", mpdevice->get_userInteractionTimeout() }};
    sendJsonMessage({{ "msg", "param_changed" }, { "data", data }});
}

void WSServerCon::sendFlashScreen()
{
    QJsonObject data = {{ "parameter", "flash_screen" },
                        { "value", mpdevice->get_flashScreen() }};
    sendJsonMessage({{ "msg", "param_changed" }, { "data", data }});
}

void WSServerCon::sendOfflineMode()
{
    QJsonObject data = {{ "parameter", "offline_mode" },
                        { "value", mpdevice->get_offlineMode() }};
    sendJsonMessage({{ "msg", "param_changed" }, { "data", data }});
}

void WSServerCon::sendTutorialEnabled()
{
    QJsonObject data = {{ "parameter", "tutorial_enabled" },
                        { "value", mpdevice->get_tutorialEnabled() }};
    sendJsonMessage({{ "msg", "param_changed" }, { "data", data }});
}

void WSServerCon::sendScreenBrightness()
{
    QJsonObject data = {{ "parameter", "screen_brightness" },
                        { "value", mpdevice->get_screenBrightness() }};
    sendJsonMessage({{ "msg", "param_changed" }, { "data", data }});
}

void WSServerCon::sendKnockEnabled()
{
    QJsonObject data = {{ "parameter", "knock_enabled" },
                        { "value", mpdevice->get_knockEnabled() }};
    sendJsonMessage({{ "msg", "param_changed" }, { "data", data }});
}

void WSServerCon::sendKnockSensitivity()
{
    QJsonObject data = {{ "parameter", "knock_sensitivity" },
                        { "value", mpdevice->get_knockSensitivity() }};
    sendJsonMessage({{ "msg", "param_changed" }, { "data", data }});
}

void WSServerCon::sendMemMgmtMode()
{
    sendJsonMessage({{ "msg", "memorymgmt_changed" },
                     { "data", mpdevice->get_memMgmtMode() }});

    QJsonArray logins;
    foreach (MPNode *n, mpdevice->getLoginNodes())
    {
        logins.append(n->toJson());
    }

    QJsonArray datas;
    foreach (MPNode *n, mpdevice->getDataNodes())
    {
        datas.append(n->toJson());
    }

    QJsonObject jdata;
    jdata["login_nodes"] = logins;
    jdata["data_nodes"] = datas;

    sendJsonMessage({{ "msg", "memorymgmt_data" },
                     { "data", jdata }});
}

void WSServerCon::sendVersion()
{
    QJsonObject data = {{ "hw_version", mpdevice->get_hwVersion() },
                        { "flash_size", mpdevice->get_flashMbSize() }};
    sendJsonMessage({{ "msg", "version_changed" }, { "data", data }});
}

void WSServerCon::processParametersSet(const QJsonObject &data)
{
    if (data.contains("keyboard_layout"))
        mpdevice->updateKeyboardLayout(data["keyboard_layout"].toInt());
    if (data.contains("lock_timeout_enabled"))
        mpdevice->updateLockTimeoutEnabled(data["lock_timeout_enabled"].toBool());
    if (data.contains("lock_timeout"))
        mpdevice->updateLockTimeout(data["lock_timeout"].toInt());
    if (data.contains("screensaver"))
        mpdevice->updateScreensaver(data["screensaver"].toBool());
    if (data.contains("user_request_cancel"))
        mpdevice->updateUserRequestCancel(data["user_request_cancel"].toBool());
    if (data.contains("user_interaction_timeout"))
        mpdevice->updateUserInteractionTimeout(data["user_interaction_timeout"].toInt());
    if (data.contains("flash_screen"))
        mpdevice->updateFlashScreen(data["flash_screen"].toBool());
    if (data.contains("offline_mode"))
        mpdevice->updateOfflineMode(data["offline_mode"].toBool());
    if (data.contains("tutorial_enabled"))
        mpdevice->updateTutorialEnabled(data["tutorial_enabled"].toBool());
    if (data.contains("screen_brightness"))
        mpdevice->updateScreenBrightness(data["screen_brightness"].toInt());
    if (data.contains("knock_enabled"))
        mpdevice->updateKnockEnabled(data["knock_enabled"].toBool());
    if (data.contains("knock_sensitivity"))
        mpdevice->updateKnockSensitivity(data["knock_sensitivity"].toInt());

    //reload parameters from device after changed all params, this will trigger
    //websocket update of clients too
    mpdevice->loadParameters();
}

QString WSServerCon::getRequestId(const QJsonValue &v)
{
    if (v.isDouble())
        return QString::number(v.toInt());
    return v.toString();
}
