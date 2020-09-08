#ifndef SRC_SYSTEMEVENTHANDLER_H
#define SRC_SYSTEMEVENTHANDLER_H

#include <QAbstractNativeEventFilter>
#include <QLibrary>
#include <QObject>
#include <QWidget>

#include "SystemEvent.h"

class SystemEventHandler : public QObject, QAbstractNativeEventFilter
{
    Q_OBJECT

public:
    SystemEventHandler();
    virtual ~SystemEventHandler();

    void emitEvent(const SystemEvent event);
    static void triggerEvent(const int type, void *instance);

    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;

signals:
    void screenLocked();
    void loggingOff();
    void goingToSleep();
    void shuttingDown();
    void screenUnlocked();

public slots:
#ifdef Q_OS_MAC
    void readyToTerminate();
#endif

private slots:
    void upstartEventEmitted(const QString &name, const QStringList &env);
    void clientPrivateEndSession(quint32 id);
    void screenSaverActiveChanged(bool on);
    void kdeAboutToSuspend();
    void login1PrepareForSleep(bool active);
    void login1PrepareForShutdown(bool active);

private:
#ifdef Q_OS_MAC
    void *eventHandler = nullptr;
#elif defined(Q_OS_WIN)
    QLibrary wtsApi32Lib;
    QWidget widget;
#endif
};

#endif // SRC_SYSTEMEVENTHANDLER_H
