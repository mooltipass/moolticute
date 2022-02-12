#ifndef SYSTEMNOTIFICATIONWINDOWS_H
#define SYSTEMNOTIFICATIONWINDOWS_H

#include "ISystemNotification.h"
#include <QProcess>

class SystemNotificationWindows : public ISystemNotification
{
    using NotificationMap = QMap<size_t, QProcess*>;
    using MessageMap = QMap<size_t, QString>;
    using CallbackType = void(QProcess::*)(int, QProcess::ExitStatus);

    enum FocusAssistResult
    {
        NOT_SUPPORTED = -2,
        FAILED = -1,
        OFF = 0,
        PRIORITY_ONLY = 1,
        ALARMS_ONLY = 2
    };

    Q_OBJECT
public:
    explicit SystemNotificationWindows(QObject *parent = nullptr);
    virtual ~SystemNotificationWindows() override;

    virtual void createNotification(const QString& title, const QString text) override;
    virtual void createButtonChoiceNotification(const QString& title, const QString text, const QStringList &buttons) override;
    virtual void createTextBoxNotification(const QString& title, const QString text) override;
    virtual bool displayLoginRequestNotification(const QString& service, QString &loginName, QString message) override;
    virtual bool displayDomainSelectionNotification(const QString& domain, const QString& subdomain, QString &serviceName, QString message) override;

    const static QString SNORETOAST_FORMAT;
    const static QString SNORETOAST_INSTALL;
    const static QString WINDOWS10_VERSION;
    const static QString NOTIFICATIONS_SETTING_REGENTRY;
    const static QString DND_ENABLED_REGENTRY;
    const static QString TOAST_ENABLED_SETTING_REGPATH;
    const static QString TOAST_ENABLED_REGENTRY;
    const static bool IS_WIN10;

signals:
    void notifySystray(QString title, QString text);

public slots:
    void callbackFunction(int exitCode, QProcess::ExitStatus exitStatus);

protected:
    bool processResult(const QString &toastResponse, QString &result, size_t &id) const;
    bool isDoNotDisturbEnabled() const;
    bool isFocusAssistEnabled() const;
    void installSnoreToast();

    QProcess* process = nullptr;
    size_t notificationId = 0;
    NotificationMap *notificationMap;
    MessageMap *messageMap;
};

#endif // SYSTEMNOTIFICATIONWINDOWS_H
