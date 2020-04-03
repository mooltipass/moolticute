#ifndef SYSTEMNOTIFICATION_H
#define SYSTEMNOTIFICATION_H

#include <QObject>
#include "ISystemNotification.h"
#include "Common.h"

class SystemNotification : QObject
{
    Q_OBJECT
    DISABLE_COPY_MOVE(SystemNotification)
private:
    explicit SystemNotification(QObject *parent = nullptr);

    ISystemNotification *m_notification = nullptr;

public:
    static SystemNotification & instance()
    {
       static SystemNotification * _instance = nullptr;
       if ( _instance == nullptr )
       {
           _instance = new SystemNotification();
       }
       return *_instance;
    }
    ~SystemNotification()
    {
        delete m_notification;
    }

    void createNotification(const QString& title, const QString text);
    void createButtonChoiceNotification(const QString& title, const QString text, const QStringList &buttons);
    void createTextBoxNotification(const QString& title, const QString text);
    bool displayLoginRequestNotification(const QString& service, QString &loginName, QString message);
    bool displayDomainSelectionNotification(const QString& domain, const QString& subdomain, QString &serviceName, QString message);
    ISystemNotification *getNotification();

};

#endif // SYSTEMNOTIFICATION_H
