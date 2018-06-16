#include "SystemEventHandler.h"

#ifdef Q_OS_MAC
#include "MacSystemEvents.h"
#elif defined(Q_OS_WIN)
#include <windows.h>
#endif

#include <QCoreApplication>
#include <QDebug>

SystemEventHandler::SystemEventHandler()
{
#ifdef Q_OS_MAC
    Q_ASSERT(!eventHandler);
    eventHandler = registerSystemHandler(this, &SystemEventHandler::triggerEvent);
#elif defined(Q_OS_WIN)
    qApp->installNativeEventFilter(this);

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
#elif defined(Q_OS_WIN)
    qApp->removeNativeEventFilter(this);
#endif
}

void SystemEventHandler::emitEvent(const SystemEvent event)
{
    switch (event)
    {
        case SCREEN_LOCKED:
            emit screenLocked();
            break;

        case LOGGING_OFF:
            emit loggingOff();
            break;

        case GOING_TO_SLEEP:
            emit goingToSleep();
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

bool SystemEventHandler::nativeEventFilter(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(result);

#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG")
    {
        const auto *msg = (MSG*) message;
        if ((msg->message == WM_ENDSESSION || msg->message == WM_QUERYENDSESSION) &&
            msg->lParam == static_cast<int>(ENDSESSION_LOGOFF))
        {
            emit loggingOff();
        }
    }
#else
    Q_UNUSED(eventType);
    Q_UNUSED(message);
#endif

  return false;
}

#ifdef Q_OS_MAC
void SystemEventHandler::readyToTerminate()
{
    ::readyToTerminate();
}
#endif
