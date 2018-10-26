#ifndef SYSTEMNOTIFICATIONUNIX_H
#define SYSTEMNOTIFICATIONUNIX_H

#include "ISystemNotification.h"
#include <QDBusInterface>

class SystemNotificationUnix : public ISystemNotification
{
public:
    explicit SystemNotificationUnix(QObject *parent = nullptr);
    virtual ~SystemNotificationUnix() override;

    virtual void createNotification(const QString& title, const QString text) override;
    virtual void createButtonChoiceNotification(const QString& title, const QString text, const QStringList &buttons) override;
    virtual void createTextBoxNotification(const QString& title, const QString text) override;
    virtual bool displayLoginRequestNotification(const QString& service, QString &loginName, QString message) override;
    virtual bool displayDomainSelectionNotification(const QString& domain, const QString& subdomain, QString &serviceName, QString message) override;

private:
    void notifyFreedesktop(const QString &title, const QString &text);

    QDBusInterface *m_interface = nullptr;
    QStringList m_actions;
    QVariantMap m_hints;

    static QString FREEDESKTOP_NOTIFICATION_SERVICE_NAME;
    static QString FREEDESKTOP_NOTIFICATION_PATH;
    static int FREEDESKTOP_NOTIFICATION_TIMEOUT;
};

#endif // SYSTEMNOTIFICATIONUNIX_H
