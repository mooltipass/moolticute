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

    //TODO
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
    connect(mpdevice, &MPDevice::statusChanged, [=]()
    {
        sendJsonMessage({{ "msg", "status_changed" },
                         { "data", Common::MPStatusString[mpdevice->get_status()] }});
    });
    connect(mpdevice, SIGNAL(keyboardLayoutChanged(int)), this, SLOT(sendKeyboardLayout()));
    connect(mpdevice, SIGNAL(lockTimeoutEnabledChanged(bool)), this, SLOT(sendLockTimeoutEnabled()));
    connect(mpdevice, SIGNAL(lockTimeoutChanged(int)), this, SLOT(sendLockTimeout()));
    connect(mpdevice, SIGNAL(screensaverChanged(bool)), this, SLOT(sendScreensaver()));
    connect(mpdevice, SIGNAL(userRequestCancelChanged(bool)), this, SLOT(sendUserRequestCancel()));
    connect(mpdevice, SIGNAL(userInteractionTimeoutChanged(int)), this, SLOT(sendUserInteractionTimeout()));
    connect(mpdevice, SIGNAL(flashScreenChanged(bool)), this, SLOT(sendFlashScreen()));
    connect(mpdevice, SIGNAL(offlineModeChanged(bool)), this, SLOT(sendOfflineMode()));
    connect(mpdevice, SIGNAL(tutorialEnabledChanged(bool)), this, SLOT(sendTutorialEnabled()));
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
