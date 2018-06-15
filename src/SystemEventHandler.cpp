#include "SystemEventHandler.h"

#ifdef Q_OS_MAC
#include "MacSystemEvents.h"
#elif defined(Q_OS_WIN)
#include <Windows.h>
#endif

#include <QDebug>

SystemEventHandler::SystemEventHandler()
{
#ifdef Q_OS_MAC
    Q_ASSERT(!eventHandler);
    eventHandler = registerSystemHandler(this, &SystemEventHandler::triggerEvent);
#elif defined(Q_OS_WIN)
    timer.setInterval(2000);
    connect(&timer, &QTimer::timeout, this, [this]
    {
        // If the following function returns nullptr then the screen is locked because you can't
        // have windows when locked.
        const auto *ret =
            OpenInputDesktop(DF_ALLOWOTHERACCOUNTHOOK, TRUE, DESKTOP_CREATEMENU |
                             DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE | DESKTOP_HOOKCONTROL |
                             DESKTOP_WRITEOBJECTS | DESKTOP_READOBJECTS | DESKTOP_SWITCHDESKTOP |
                             GENERIC_WRITE);
        if (!ret && !screenLocked_) {
            screenLocked_ = true;
            emit screenLocked();
        }
        else if (ret) {
            screenLocked_ = false;
        }
    });
    timer.start();
#endif
}

SystemEventHandler::~SystemEventHandler()
{
#ifdef Q_OS_MAC
    Q_ASSERT(eventHandler);
    unregisterSystemHandler(eventHandler);
#endif
}

void SystemEventHandler::emitEvent(const SystemEvent event)
{
    switch (event) {
    case SCREEN_LOCKED:
        emit screenLocked();
        break;

    default:
        qCritical() << "Unknown system event:" << event;
        Q_ASSERT(false);
        break;
    }
}

void SystemEventHandler::triggerEvent(const int type, void *instance)
{
    auto *handler = static_cast<SystemEventHandler*>(instance);
    if (handler) {
        handler->emitEvent(static_cast<SystemEvent>(type));
    }
}
