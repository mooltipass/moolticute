#ifndef SYSTEMNOTIFICATIONWINDOWS_H
#define SYSTEMNOTIFICATIONWINDOWS_H

#include "ISystemNotification.h"
#include <QProcess>

class SystemNotificationWindows : public ISystemNotification
{
    Q_OBJECT
public:
    explicit SystemNotificationWindows(QObject *parent = nullptr);
    virtual ~SystemNotificationWindows() override;

    virtual void createNotification(const QString& title, const QString text) override;
    virtual void createButtonChoiceNotification(const QString& title, const QString text, const QStringList &buttons) override;
    virtual void createTextBoxNotification(const QString& title, const QString text) override;
    virtual bool displayLoginRequestNotification(const QString& service, QString &loginName) override;
    virtual bool displayDomainSelectionNotification(const QString& domain, const QString& subdomain, QString &serviceName) override;

    const static QString SNORETOAST_FORMAT;
    const static QString WINDOWS10_VERSION;

protected:
    bool processResult(const QString &toastResponse, QString &result) const;

    QProcess* process = nullptr;
};

#endif // SYSTEMNOTIFICATIONWINDOWS_H
