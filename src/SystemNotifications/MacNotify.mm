#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>
#include <QDebug>

#import <objc/runtime.h>

#include "MacNotify.h"

static inline QString toQString(NSString *string)
{
    if (!string)
        return QString();
    return QString::fromUtf8([string UTF8String]);
}

@interface MacNotificationCenterDelegate : NSObject <NSUserNotificationCenterDelegate> {
    MacNotify* MacNotifyObserver;
}
    - (MacNotificationCenterDelegate*) initialise:(MacNotify*)observer;
    - (void) userNotificationCenter:(NSUserNotificationCenter *)center didActivateNotification:(NSUserNotification *)notification;
    - (void) userNotificationCenter:(NSUserNotificationCenter *)center didDismissAlert:(NSUserNotification *)notification;
    - (BOOL) userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification;
@end

@implementation MacNotificationCenterDelegate
- (MacNotificationCenterDelegate*) initialise:(MacNotify*)observer
{
    if ( (self = [super init]) )
        self->MacNotifyObserver = observer;
    return self;
}

- (void) userNotificationCenter:(NSUserNotificationCenter *)center didActivateNotification:(NSUserNotification *)notification
{
    Q_UNUSED(center);
    QString result = "";
    if (notification.additionalActions != nil)
    {
        result = toQString(notification.additionalActivationAction.title);
        qDebug() << "additionalActions: " << result;
    }

    if (notification.response != nil)
    {
        result = toQString(notification.response.string);
        qDebug() << "Respone:  " << result;
    }
    if (!result.isEmpty())
    {
        self->MacNotifyObserver->notificationClicked(result);
        self->MacNotifyObserver->resultSet();
    }
}

- (void) userNotificationCenter:(NSUserNotificationCenter *)center didDismissAlert:(NSUserNotification *)notification
{
    Q_UNUSED(center);
    Q_UNUSED(notification);
    self->MacNotifyObserver->dismissedNotification();
}


- (BOOL) userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification
{
    Q_UNUSED(center);
    Q_UNUSED(notification);
    return YES;
}

@end

MacNotify::MacNotify(QObject *parent) : QObject(parent)
{
    MacNotificationCenterDelegate* macDelegate = [[MacNotificationCenterDelegate alloc] initialise: this];
    MacNotificationCenterWrapped = macDelegate;
    [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:macDelegate];
    [[NSUserNotificationCenter defaultUserNotificationCenter] removeAllDeliveredNotifications];
}

MacNotify::~MacNotify()
{
    [[NSUserNotificationCenter defaultUserNotificationCenter] removeAllDeliveredNotifications];
}

void MacNotify::showNotification(const QString &title, const QString &text)
{
    NSUserNotification *userNotification = [[NSUserNotification alloc] init];
    userNotification.title = title.toNSString();
    userNotification.subtitle = text.toNSString();
    userNotification.soundName = NSUserNotificationDefaultSoundName;

    [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:userNotification];
}

void MacNotify::showButtonNotification(const QString &title, const QString &text, const QStringList &buttons)
{
    NSUserNotification *userNotification = [[NSUserNotification alloc] init];
    userNotification.title = title.toNSString();
    userNotification.subtitle = text.toNSString();
    userNotification.soundName = NSUserNotificationDefaultSoundName;
    userNotification.hasActionButton = true;
    userNotification.actionButtonTitle = QString(tr("Choose")).toNSString();
    NSMutableArray<NSUserNotificationAction *> *actions = [[NSMutableArray<NSUserNotificationAction *> alloc] init];

    for (const QString &button : buttons)
    {
        [actions addObject:[NSUserNotificationAction actionWithIdentifier:button.toNSString() title:button.toNSString()]];
    }

    userNotification.additionalActions = actions;
    [userNotification setValue:[NSNumber numberWithBool:YES] forKey:@"_alwaysShowAlternateActionMenu"];
    [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:userNotification];
}

void MacNotify::showTextBoxNotification(const QString &title, const QString &text)
{
    NSUserNotification *userNotification = [[NSUserNotification alloc] init];
    userNotification.title = title.toNSString();
    userNotification.subtitle = text.toNSString();
    userNotification.soundName = NSUserNotificationDefaultSoundName;
    userNotification.hasReplyButton = true;
    userNotification.responsePlaceholder = QString(tr("Type your reply here")).toNSString();

    [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:userNotification];
}

void MacNotify::notificationClicked(QString result)
{
    emit clicked(result);
}

