#include "SystemNotification.h"
#ifdef Q_OS_WIN
#include "SystemNotificationWindows.h"
#endif

SystemNotification::SystemNotification(QObject *parent)
    : QObject(parent)
{
#ifdef Q_OS_WIN
     notification = new SystemNotificationWindows(this);
#endif
}

void SystemNotification::createNotification(const QString &title, const QString text)
{
    if (notification)
    {
        notification->createNotification(title, text);
    }
}

void SystemNotification::createButtonChoiceNotification(const QString &title, const QString text, const QStringList &buttons)
{
    if (notification)
    {
        notification->createButtonChoiceNotification(title, text, buttons);
    }
}

void SystemNotification::createTextBoxNotification(const QString &title, const QString text)
{
    if (notification)
    {
        notification->createTextBoxNotification(title, text);
    }
}

bool SystemNotification::displayLoginRequestNotification(const QString &service, QString& loginName)
{
    if (notification)
    {
        return notification->displayLoginRequestNotification(service, loginName);
    }
    return false;
}
