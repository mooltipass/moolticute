#include "SystemNotificationUnix.h"
#include "RequestLoginNameDialog.h"
#include "RequestDomainSelectionDialog.h"
#include "SystemNotificationImageUnix.h"

#include <QException>
#include <QDBusPendingReply>
#include <QDebug>
#include <QIcon>

QString SystemNotificationUnix::FREEDESKTOP_NOTIFICATION_SERVICE_NAME = "org.freedesktop.Notifications";
QString SystemNotificationUnix::FREEDESKTOP_NOTIFICATION_PATH = "/org/freedesktop/Notifications";
int SystemNotificationUnix::FREEDESKTOP_NOTIFICATION_TIMEOUT = 5000;

SystemNotificationUnix::SystemNotificationUnix(QObject *parent)
    :ISystemNotification(parent)
{
    m_interface = new QDBusInterface(FREEDESKTOP_NOTIFICATION_SERVICE_NAME, FREEDESKTOP_NOTIFICATION_PATH,
                                     FREEDESKTOP_NOTIFICATION_SERVICE_NAME, QDBusConnection::sessionBus(), this);
    SystemNotificationImageUnix appImage(QIcon(":/AppIcon.icns").pixmap(QSize(128, 128)).toImage());
    m_hints.insert(QStringLiteral("image_data"), QVariant::fromValue(appImage));
    m_hints.insert(QStringLiteral("urgency"), 1);
    m_hints.insert(QStringLiteral("suppress-sound"), false);
}

SystemNotificationUnix::~SystemNotificationUnix()
{
    delete m_interface;
}

void SystemNotificationUnix::createNotification(const QString &title, const QString text)
{
    notifyFreedesktop(title, text);
}

void SystemNotificationUnix::createButtonChoiceNotification(const QString &title, const QString text, const QStringList &buttons)
{
    QStringList origActions = m_actions;
    int buttonNumber = 0;
    for (const QString &button : buttons)
    {
        m_actions << QString::number(buttonNumber++) << button;
    }
    notifyFreedesktop(title, text);
    m_actions = origActions;
}

void SystemNotificationUnix::createTextBoxNotification(const QString &title, const QString text)
{
    Q_UNUSED(title);
    Q_UNUSED(text);
    Q_UNIMPLEMENTED();
}

bool SystemNotificationUnix::displayLoginRequestNotification(const QString &service, QString &loginName)
{
    bool isSuccess = false;
    RequestLoginNameDialog dlg(service);
    isSuccess = (dlg.exec() != QDialog::Rejected);
    loginName = dlg.getLoginName();
    return isSuccess;
}

bool SystemNotificationUnix::displayDomainSelectionNotification(const QString &domain, const QString &subdomain, QString &serviceName)
{
    bool isSuccess = false;
    RequestDomainSelectionDialog dlg(domain, subdomain);
    isSuccess = (dlg.exec() != QDialog::Rejected);
    serviceName = dlg.getServiceName();
    return isSuccess;
}

void SystemNotificationUnix::notifyFreedesktop(const QString &title, const QString &text)
{
    QList<QVariant> argumentList;
    argumentList << "Moolticute" << uint(0) << "" << title << text << m_actions << m_hints << FREEDESKTOP_NOTIFICATION_TIMEOUT;
    QDBusPendingReply<uint> id = m_interface->asyncCallWithArgumentList("Notify", argumentList);
    id.waitForFinished();
}
