#include "SystemNotification.h"

#ifdef Q_OS_WIN
#include "SystemNotificationWindows.h"
#endif
#ifdef Q_OS_MAC
#include "SystemNotificationMac.h"
#endif

SystemNotification::SystemNotification(QObject *parent)
    : QObject(parent)
{
#ifdef Q_OS_WIN
     m_notification = new SystemNotificationWindows(this);
#endif
#ifdef Q_OS_MAC
     m_notification = new SystemNotificationMac(this);
#endif
}

void SystemNotification::createNotification(const QString &title, const QString text)
{
    if (m_notification)
    {
        m_notification->createNotification(title, text);
    }
}

void SystemNotification::createButtonChoiceNotification(const QString &title, const QString text, const QStringList &buttons)
{
    if (m_notification)
    {
        m_notification->createButtonChoiceNotification(title, text, buttons);
    }
}

void SystemNotification::createTextBoxNotification(const QString &title, const QString text)
{
    if (m_notification)
    {
        m_notification->createTextBoxNotification(title, text);
    }
}

bool SystemNotification::displayLoginRequestNotification(const QString &service, QString& loginName)
{
    if (m_notification)
    {
        return m_notification->displayLoginRequestNotification(service, loginName);
    }
    return false;
}

bool SystemNotification::displayDomainSelectionNotification(const QString &domain, const QString &subdomain, QString &serviceName)
{
    if (m_notification)
    {
        return m_notification->displayDomainSelectionNotification(domain, subdomain, serviceName);
    }
    return false;
}
