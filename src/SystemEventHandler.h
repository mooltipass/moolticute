#ifndef SRC_SYSTEMEVENTHANDLER_H
#define SRC_SYSTEMEVENTHANDLER_H

#include <QAbstractNativeEventFilter>
#include <QObject>
#include <QTimer>

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

public slots:
#ifdef Q_OS_MAC
    void readyToTerminate();
#endif

private:
#ifdef Q_OS_MAC
    void *eventHandler = nullptr;
#elif defined(Q_OS_WIN)
    QTimer timer;
    bool screenLocked_ = false;
#endif
};

#endif // SRC_SYSTEMEVENTHANDLER_H
