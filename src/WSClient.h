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

    QT_WRITABLE_PROPERTY(bool, connected)
    QT_WRITABLE_PROPERTY(Common::MPStatus, status)

public:
    explicit WSClient(QObject *parent = nullptr);
    ~WSClient();

signals:
    void wsConnected();
    void wsDisconnected();

private slots:
    void onWsConnected();
    void onWsDisconnected();
    void onWsError();
    void onTextMessageReceived(const QString &message);
    void sendJsonData(const QJsonObject &data);

private:
    void openWebsocket();
    void closeWebsocket();

    QWebSocket *wsocket = nullptr;
};

#endif // WSCLIENT_H
