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

    void notificationClicked(QString result, int notificationId, QString jsonMessage);
    void showNotification(const QString &title, const QString &text);
    void showButtonNotification(const QString &title, const QString &text, const QStringList &buttons, int notifyId, const QString &jsonMsg);
    void showTextBoxNotification(const QString &title, const QString &text, int notifyId, const QString &jsonMsg);

signals:
    void clicked(QString result, int notificationId, QString jsonMessage);
    void dismissedNotification(int notificationId, QString jsonMessage);

private:
    MacNotificationCenterDelegate *MacNotificationCenterWrapped;

};

#endif // MACNOTIFY_H
