#include "MacSystemEvents.h"
#include "SystemEvent.h"

#import <AppKit/AppKit.h>

@interface MacSystemEventsHandler : NSObject

@property(assign) void *instance;
@property(assign) void (*trigger)(const int, void *);

- (void)screenLocked:(NSNotification *)notif;

@end

@implementation MacSystemEventsHandler

- (id)init
{
    id newSelf = [super init];
    if (newSelf) {
        id center = [NSDistributedNotificationCenter defaultCenter];
        [center addObserver:newSelf
                   selector:@selector(screenLocked:)
                       name:@"com.apple.screenIsLocked"
                     object:nil];
    }
    return newSelf;
}

- (void)screenLocked:(NSNotification *)notif
{
    (void) notif;
    _trigger(SCREEN_LOCKED, _instance);
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
        [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:handler];
        [handler dealloc];
    }
}
