#include "SystemNotificationMac.h"
#include "MacUtils.h"
#include <QDebug>
#include <QEventLoop>
#include <QTimer>

SystemNotificationMac::SystemNotificationMac(QObject *parent)
    : ISystemNotification(parent)
{
    m_macNotify = new MacNotify(this);
    connect(m_macNotify, &MacNotify::clicked, this, &SystemNotificationMac::setResult);
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
        m_macNotify->showButtonNotification(title, text, buttons);
    }
    else
    {
        qDebug() << "No button to display";
    }
}

void SystemNotificationMac::createTextBoxNotification(const QString &title, const QString text)
{
    m_macNotify->showTextBoxNotification(title, text);
}

bool SystemNotificationMac::displayLoginRequestNotification(const QString &service, QString &loginName)
{
    createTextBoxNotification(tr("A credential without a login has been detected."), tr("Login name for ") + service + ":");
    return waitForNotification(loginName);
}

bool SystemNotificationMac::displayDomainSelectionNotification(const QString &domain, const QString &subdomain, QString &serviceName)
{
    QStringList buttons;
    buttons.append({domain, subdomain});
    createButtonChoiceNotification(tr("Subdomain Detected!"), tr("Choose the domain name:"), buttons);
    return waitForNotification(serviceName);
}

void SystemNotificationMac::setResult(QString result)
{
    m_result = result;
}

bool SystemNotificationMac::waitForNotification(QString &result)
{
    QEventLoop loop;
    QTimer timer;
    connect(m_macNotify, SIGNAL(resultSet()), &loop, SLOT(quit()));
    connect(m_macNotify, SIGNAL(dismissedNotification()), &loop, SLOT(quit()));
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    timer.start(NOTIFICATION_TIMEOUT);
    loop.exec(); //blocks until result is set or timeout or notification is dismissed
    if (!m_result.isEmpty())
    {
        result = m_result;
        m_result = "";
        timer.stop();
        return true;
    }
    else
    {
        qDebug() << "Notification is dismissed";
        return true;
    }
}
