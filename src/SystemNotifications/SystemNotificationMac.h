#ifndef SYSTEMNOTIFICATIONMAC_H
#define SYSTEMNOTIFICATIONMAC_H

#include "ISystemNotification.h"
#include "MacNotify.h"

class SystemNotificationMac : public ISystemNotification
{

public:
    explicit SystemNotificationMac(QObject *parent = nullptr);
    virtual ~SystemNotificationMac() override;

    virtual void createNotification(const QString& title, const QString text) override;
    virtual void createButtonChoiceNotification(const QString& title, const QString text, const QStringList &buttons) override;
    virtual void createTextBoxNotification(const QString& title, const QString text) override;
    virtual bool displayLoginRequestNotification(const QString& service, QString &loginName, QString message) override;
    virtual bool displayDomainSelectionNotification(const QString& domain, const QString& subdomain, QString &serviceName, QString message) override;

signals:
    void resultSet();

public slots:
    void setResult(QString result);

private:
    bool waitForNotification(QString &result);


    MacNotify *m_macNotify = nullptr;
    QString m_result = "";

};

#endif // SYSTEMNOTIFICATIONMAC_H
