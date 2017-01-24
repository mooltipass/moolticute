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

    QT_WRITABLE_PROPERTY(bool, connected, false)
    QT_WRITABLE_PROPERTY(Common::MPStatus, status, Common::UnknownStatus)
    QT_WRITABLE_PROPERTY(int, keyboardLayout, 0)
    QT_WRITABLE_PROPERTY(bool, lockTimeoutEnabled, false)
    QT_WRITABLE_PROPERTY(int, lockTimeout, 0)
    QT_WRITABLE_PROPERTY(bool, screensaver, false)
    QT_WRITABLE_PROPERTY(bool, userRequestCancel, false)
    QT_WRITABLE_PROPERTY(int, userInteractionTimeout, 0)
    QT_WRITABLE_PROPERTY(bool, flashScreen, false)
    QT_WRITABLE_PROPERTY(bool, offlineMode, false)
    QT_WRITABLE_PROPERTY(bool, tutorialEnabled, false)
    QT_WRITABLE_PROPERTY(int, screenBrightness, 0)
    QT_WRITABLE_PROPERTY(bool, knockEnabled, false)
    QT_WRITABLE_PROPERTY(int, knockSensitivity, 0)
    QT_WRITABLE_PROPERTY(bool, memMgmtMode, false)

    QT_WRITABLE_PROPERTY(Common::MPHwVersion, mpHwVersion, Common::MP_Classic)

    QT_WRITABLE_PROPERTY(bool, keyAfterLoginSendEnable, false)
    QT_WRITABLE_PROPERTY(int, keyAfterLoginSend, 0)
    QT_WRITABLE_PROPERTY(bool, keyAfterPassSendEnable, false)
    QT_WRITABLE_PROPERTY(int, keyAfterPassSend, 0)
    QT_WRITABLE_PROPERTY(bool, delayAfterKeyEntryEnable, false)
    QT_WRITABLE_PROPERTY(int, delayAfterKeyEntry, 0)

public:
    explicit WSClient(QObject *parent = nullptr);
    ~WSClient();

    void closeWebsocket();

    QJsonObject &getMemoryData() { return memData; }

    bool isMPMini();

signals:
    void wsConnected();
    void wsDisconnected();
    void memoryDataChanged();
    void askPasswordDone(bool success, const QString &pass);
    void addCredentialDone(bool success);
    void showAppRequested();

public slots:
    void sendJsonData(const QJsonObject &data);

private slots:
    void onWsConnected();
    void onWsDisconnected();
    void onWsError();
    void onTextMessageReceived(const QString &message);

private:
    void openWebsocket();

    void udateParameters(const QJsonObject &data);

    QWebSocket *wsocket = nullptr;

    QJsonObject memData;
};

#endif // WSCLIENT_H
