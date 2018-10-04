#ifndef MACNOTIFY_H
#define MACNOTIFY_H

#include <QObject>
#include <QString>

#ifdef __OBJC__
@class MacNotificationCenterDelegate;
#else
typedef struct objc_object MacNotificationCenterDelegate;
#endif

class MacNotify : public QObject
{
	Q_OBJECT
public:
    explicit MacNotify(QObject *parent = nullptr);
	~MacNotify();

    void notificationClicked(QString result);
    void showNotification(const QString &title, const QString &text);
    void showButtonNotification(const QString &title, const QString &message, const QStringList &buttons);
    void showTextBoxNotification(const QString &title, const QString &message);

signals:
    void clicked(QString result);
    void resultSet();
    void dismissedNotification();

private:
    MacNotificationCenterDelegate *MacNotificationCenterWrapped;

};

#endif // MACNOTIFY_H
