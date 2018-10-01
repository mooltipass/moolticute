#ifndef SYSTEMNOTIFICATION_H
#define SYSTEMNOTIFICATION_H

#include <QObject>
#include <ISystemNotification.h>

class SystemNotification : QObject
{
    Q_OBJECT
private:
    explicit SystemNotification(QObject *parent = nullptr);
    SystemNotification(const SystemNotification&) = delete;
    SystemNotification(const SystemNotification&&) = delete;
    SystemNotification& operator=(const SystemNotification& other) = delete;
    SystemNotification& operator=(const SystemNotification&& other) = delete;

    ISystemNotification *notification = nullptr;

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
        delete notification;
    }

    void createNotification(const QString& title, const QString text);
    void createButtonChoiceNotification(const QString& title, const QString text, const QStringList &buttons);
    void createTextBoxNotification(const QString& title, const QString text);
    bool displayLoginRequestNotification(const QString& service, QString &loginName);
};

#endif // SYSTEMNOTIFICATION_H
