#ifndef SRC_SYSTEMEVENTHANDLER_H
#define SRC_SYSTEMEVENTHANDLER_H

#include <QObject>

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
    void init();
    void uninit();

    void *eventHandler = nullptr;
};

#endif // SRC_SYSTEMEVENTHANDLER_H
