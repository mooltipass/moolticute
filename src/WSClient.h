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
    QT_WRITABLE_PROPERTY(int, keyboardLayout)
    QT_WRITABLE_PROPERTY(bool, lockTimeoutEnabled)
    QT_WRITABLE_PROPERTY(int, lockTimeout)
    QT_WRITABLE_PROPERTY(bool, screensaver)
    QT_WRITABLE_PROPERTY(bool, userRequestCancel)
    QT_WRITABLE_PROPERTY(int, userInteractionTimeout)
    QT_WRITABLE_PROPERTY(bool, flashScreen)
    QT_WRITABLE_PROPERTY(bool, offlineMode)
    QT_WRITABLE_PROPERTY(bool, tutorialEnabled)
    QT_WRITABLE_PROPERTY(bool, memMgmtMode)

public:
    explicit WSClient(QObject *parent = nullptr);
    ~WSClient();

signals:
    void wsConnected();
    void wsDisconnected();

public slots:
    void sendJsonData(const QJsonObject &data);

private slots:
    void onWsConnected();
    void onWsDisconnected();
    void onWsError();
    void onTextMessageReceived(const QString &message);

private:
    void openWebsocket();
    void closeWebsocket();

    void udateParameters(const QJsonObject &data);

    QWebSocket *wsocket = nullptr;
};

#endif // WSCLIENT_H
