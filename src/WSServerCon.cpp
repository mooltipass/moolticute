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
    }
}
