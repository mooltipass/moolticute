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

protected:
    bool processResult(const QString &toastResponse, QString &result) const;

    QProcess* process = nullptr;
    const QString SNORETOAST_FORMAT= "SnoreToast.exe -t \"%1\" -m \"%2\" %3 -p icon.png -w";
};

#endif // SYSTEMNOTIFICATIONWINDOWS_H
