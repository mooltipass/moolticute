#ifndef SRC_SYSTEMEVENTHANDLER_H
#define SRC_SYSTEMEVENTHANDLER_H

#include <QObject>
#include <QTimer>

#include "SystemEvent.h"

class SystemEventHandler : public QObject
{
    Q_OBJECT

public:
    SystemEventHandler();
    virtual ~SystemEventHandler();

    void emitEvent(const SystemEvent event);
    static void triggerEvent(const int type, void *instance);

signals:
    void screenLocked();

private:
#ifdef Q_OS_MAC
    void *eventHandler = nullptr;
#elif defined(Q_OS_WIN)
    QTimer timer;
    bool screenLocked_ = false;
#endif
};

#endif // SRC_SYSTEMEVENTHANDLER_H
