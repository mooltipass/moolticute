#include "WSServerCon.h"

WSServerCon::WSServerCon(QWebSocket *conn):
    wsClient(conn)
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
    else if (root["msg"] == "ask_password")
    {
        QJsonObject o = root["data"].toObject();
        mpdevice->askPassword(o["service"].toString(), o["login"].toString(),
                [=](bool success, const QString &login, const QString &pass)
        {
            QJsonObject ores = o;
            QJsonObject oroot = root;
            if (success)
            {
                ores["login"] = login;
                ores["password"] = pass;
                oroot["data"] = ores;
                sendJsonMessage(oroot);
            }
            else
            {
                ores["failed"] = true;
                oroot["data"] = ores;
                sendJsonMessage(oroot);
            }
        });
    }
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

    //reload parameters from device after changed all params, this will trigger
    //websocket update of clients too
    mpdevice->loadParameters();
}
