#include "SystemNotificationMac.h"
#include "MacUtils.h"
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

SystemNotificationMac::SystemNotificationMac(QObject *parent)
    : ISystemNotification(parent)
{
    m_macNotify = new MacNotify(this);
    connect(m_macNotify, &MacNotify::clicked, this, &SystemNotificationMac::notificationResponse);
    connect(m_macNotify, &MacNotify::dismissedNotification, this, &SystemNotificationMac::dismissedNotificationRespone);
}

SystemNotificationMac::~SystemNotificationMac()
{
    delete m_macNotify;
}

void SystemNotificationMac::createNotification(const QString &title, const QString text)
{
    m_macNotify->showNotification(title, text);
}

void SystemNotificationMac::createButtonChoiceNotification(const QString &title, const QString text, const QStringList &buttons)
{
    if (!buttons.empty())
    {
        m_macNotify->showButtonNotification(title, text, buttons, notificationId++, "");
    }
    else
    {
        qDebug() << "No button to display";
    }
}

void SystemNotificationMac::createTextBoxNotification(const QString &title, const QString text)
{
    m_macNotify->showTextBoxNotification(title, text, notificationId++, "");
}

bool SystemNotificationMac::displayLoginRequestNotification(const QString &service, QString &loginName, QString message)
{
    Q_UNUSED(loginName);
    m_macNotify->showTextBoxNotification(tr("A credential without a login has been detected."), tr("Login name for ") + service + ":", notificationId++, message);
    return false;
}

bool SystemNotificationMac::displayDomainSelectionNotification(const QString &domain, const QString &subdomain, QString &serviceName, QString message)
{
    Q_UNUSED(serviceName);
    QStringList buttons;
    buttons.append({domain, subdomain});
    m_macNotify->showButtonNotification(tr("Subdomain Detected!"), tr("Choose the domain name:"), buttons, notificationId++, message);
    return false;
}

void SystemNotificationMac::notificationResponse(QString result, int notificationId, QString jsonMsg)
{
    qDebug() << "Notification response for: " << notificationId;
    if (jsonMsg.contains("request_login"))
    {
        emit sendLoginMessage(jsonMsg, result);
    }
    else if (jsonMsg.contains("request_domain"))
    {
        emit sendDomainMessage(jsonMsg, result);
    }
}

void SystemNotificationMac::dismissedNotificationRespone(int notificationId, QString jsonMsg)
{
    qDebug() << notificationId << " is dismissed";
    if (jsonMsg.contains("request_login"))
    {
        emit sendLoginMessage(jsonMsg, "");
    }
    else if (jsonMsg.contains("request_domain"))
    {
        emit sendDomainMessage(jsonMsg, "");
    }
}
