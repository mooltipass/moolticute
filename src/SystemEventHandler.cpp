#include "SystemEventHandler.h"

#ifdef Q_OS_MAC
#include "MacSystemEvents.h"
#elif defined(Q_OS_WIN)
#include <windows.h>
#include <wtsapi32.h>
#elif defined(Q_OS_LINUX)
#include <QtDBus/QtDBus>
#endif

#include <QCoreApplication>
#include <QDebug>

SystemEventHandler::SystemEventHandler()
#ifdef Q_OS_WIN
    : wtsApi32Lib("wtsapi32")
#endif
{
#ifdef Q_OS_MAC
    Q_ASSERT(!eventHandler);
    eventHandler = registerSystemHandler(this, &SystemEventHandler::triggerEvent);
#elif defined(Q_OS_WIN)
    qApp->installNativeEventFilter(this);

    if (wtsApi32Lib.load())
    {
        typedef WINBOOL (*RegFunc)(HWND, DWORD);
        const auto regFunc = (RegFunc) wtsApi32Lib.resolve("WTSRegisterSessionNotification");
        if (regFunc) {
            regFunc((HWND) widget.winId(), 0);
        }
    }
#elif defined(Q_OS_LINUX)
    /* Session events */
    auto bus = QDBusConnection::sessionBus();

    // Catch Ubuntu events.
    bus.connect("", "/com/ubuntu/Upstart", "", "EventEmitted", "sas", this,
                SLOT(upstartEventEmitted(QString,QStringList)));

    // Catch Gnome shutdown events.
    bus.connect("", "", "org.gnome.SessionManager.ClientPrivate", "EndSession", "u", this,
                SLOT(clientPrivateEndSession(quint32)));

    // Catch org.freedesktop.ScreenSaver event (Used with KDE at least).
    bus.connect("", "/org/freedesktop/ScreenSaver", "", "ActiveChanged", "b", this,
                SLOT(screenSaverActiveChanged(bool)));

    // Catch org.gnome.ScreenSaver event
    bus.connect("", "/org/gnome/ScreenSaver", "", "ActiveChanged", "b", this,
                SLOT(screenSaverActiveChanged(bool)));

    // Catch KDE about-to-suspend event.
    bus.connect("", "/org/kde/Solid/PowerManagement/Actions/SuspendSession", "", "aboutToSuspend",
                this, SLOT(kdeAboutToSuspend()));

    /* System events */
    auto sysBus = QDBusConnection::systemBus();

    // Catch suspend event from org.freedesktop.login1.
    sysBus.connect("", "/org/freedesktop/login1", "", "PrepareForSleep", "b", this,
                   SLOT(login1PrepareForSleep(bool)));

    // Catch shutdown event from org.freedesktop.login1.
    sysBus.connect("", "/org/freedesktop/login1", "", "PrepareForShutdown", "b", this,
                   SLOT(login1PrepareForShutdown(bool)));
#endif
}

SystemEventHandler::~SystemEventHandler()
{
#ifdef Q_OS_MAC
    Q_ASSERT(eventHandler);
    unregisterSystemHandler(eventHandler);
#elif defined(Q_OS_WIN)
    qDebug() << "Closing SystemEventHandler";
    qApp->removeNativeEventFilter(this);

    if (wtsApi32Lib.isLoaded())
    {
        typedef WINBOOL (*UnRegFunc)(HWND);
        const auto unRegFunc = (UnRegFunc) wtsApi32Lib.resolve("WTSUnRegisterSessionNotification");
        if (unRegFunc) {
            unRegFunc((HWND) widget.winId());
        }
    }
#elif defined(Q_OS_LINUX)
    auto bus = QDBusConnection::sessionBus();
    bus.disconnect("", "/com/ubuntu/Upstart", "", "EventEmitted", "sas", this,
                SLOT(upstartEventEmitted(QString,QStringList)));
    bus.disconnect("", "", "org.gnome.SessionManager.ClientPrivate", "EndSession", "u", this,
                SLOT(clientPrivateEndSession(quint32)));
    bus.disconnect("", "/org/freedesktop/ScreenSaver", "", "ActiveChanged", "b", this,
                SLOT(screenSaverActiveChanged(bool)));
    bus.disconnect("", "/org/gnome/ScreenSaver", "", "ActiveChanged", "b", this,
                SLOT(screenSaverActiveChanged(bool)));
    bus.disconnect("", "/org/kde/Solid/PowerManagement/Actions/SuspendSession", "", "aboutToSuspend",
                this, SLOT(kdeAboutToSuspend()));

    auto sysBus = QDBusConnection::systemBus();
    sysBus.disconnect("", "/org/freedesktop/login1", "", "PrepareForSleep", "b", this,
                      SLOT(login1PrepareForSleep(bool)));
    sysBus.connect("", "/org/freedesktop/login1", "", "PrepareForShutdown", "b", this,
                   SLOT(login1PrepareForShutdown(bool)));
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

       case SHUTTING_DOWN:
            emit shuttingDown();
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
        if (msg->message == WM_ENDSESSION || msg->message == WM_QUERYENDSESSION)
        {
            if (msg->lParam == static_cast<int>(ENDSESSION_LOGOFF))
            {
                emit loggingOff();
            }
            else
            {
                emit shuttingDown();
            }
        }
        else if (msg->message == WM_POWERBROADCAST && msg->wParam == PBT_APMSUSPEND)
        {
            emit goingToSleep();
        }
        else if (msg->message == WM_WTSSESSION_CHANGE && msg->wParam == WTS_SESSION_LOCK)
        {
            emit screenLocked();
        }
        else if (msg->message == WM_WTSSESSION_CHANGE && msg->wParam == WTS_SESSION_UNLOCK)
        {
            emit screenUnlocked();
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

void SystemEventHandler::upstartEventEmitted(const QString &name, const QStringList &env)
{
    if (name == "desktop-lock")
    {
        emit screenLocked();
    }
    else if (name == "session-end" && env.size() == 1 && env.first().toLower() == "type=shutdown")
    {
        emit shuttingDown();
    }
}

void SystemEventHandler::clientPrivateEndSession(quint32 id)
{
    Q_UNUSED(id);
    emit loggingOff();
}

void SystemEventHandler::screenSaverActiveChanged(bool on)
{
    if (on)
    {
        emit screenLocked();
    }
}

void SystemEventHandler::kdeAboutToSuspend()
{
    emit goingToSleep();
}

void SystemEventHandler::login1PrepareForSleep(bool active)
{
    if (active)
    {
        emit goingToSleep();
    }
}

void SystemEventHandler::login1PrepareForShutdown(bool active)
{
    if (active)
    {
        emit shuttingDown();
    }
}
