#include "MacSystemEvents.h"
#include "SystemEvent.h"

#import <AppKit/AppKit.h>

@interface MacSystemEventsHandler : NSObject

@property(assign) void *instance;
@property(assign) void (*trigger)(const int, void *);

- (void)screenLocked:(NSNotification *)notif;
- (void)loggingOff:(NSNotification *)notif;
- (void)goingToSleep:(NSNotification *)notif;

@end

@implementation MacSystemEventsHandler

- (id)init
{
    id newSelf = [super init];
    if (newSelf) {
        [NSApp setDelegate:newSelf]; // To receive applicationShouldTerminate.

        id center = [NSDistributedNotificationCenter defaultCenter];
        [center addObserver:newSelf
                   selector:@selector(screenLocked:)
                       name:@"com.apple.screenIsLocked"
                     object:nil];

        id notifCenter = [[NSWorkspace sharedWorkspace] notificationCenter];
        [notifCenter addObserver:newSelf
                        selector:@selector(loggingOff:)
                            name:NSWorkspaceSessionDidResignActiveNotification
                          object:nil];

        [notifCenter addObserver:newSelf
                        selector:@selector(goingToSleep:)
                            name:NSWorkspaceWillSleepNotification
                          object:nil];
    }
    return newSelf;
}

- (void)screenLocked:(NSNotification *)notif
{
    (void) notif;
    _trigger(SCREEN_LOCKED, _instance);
}

- (void)loggingOff:(NSNotification *)notif
{
    (void) notif;
    _trigger(LOGGING_OFF, _instance);
}

- (void)goingToSleep:(NSNotification *)notif
{
    (void) notif;
    _trigger(GOING_TO_SLEEP, _instance);
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    (void) sender;

    id eventMgr = [NSAppleEventManager sharedAppleEventManager];
    id reason = [[eventMgr currentAppleEvent] attributeDescriptorForKeyword:kAEQuitReason];
    if (reason && [reason descriptorType] == typeType)
    {
        switch ([reason typeCodeValue])
        {
            case kAELogOut:
            case kAEReallyLogOut:
              _trigger(LOGGING_OFF, _instance);

              // Give time to send lock message to device.
              return NSTerminateLater;

            default:
              break;
        }
    }

    return NSTerminateNow;
}

@end

void *registerSystemHandler(void *instance, void (*trigger)(const int, void *))
{
    id handler = [[MacSystemEventsHandler alloc] init];
    [handler setTrigger:trigger];
    [handler setInstance:instance];
    return (void*) handler;
}

void unregisterSystemHandler(void *instance)
{
    id handler = (MacSystemEventsHandler*) instance;
    if (handler)
    {
        [[NSDistributedNotificationCenter defaultCenter] removeObserver:handler];
        [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:handler];
        [handler dealloc];
    }
}

void readyToTerminate()
{
    [NSApp replyToApplicationShouldTerminate:YES];
}
