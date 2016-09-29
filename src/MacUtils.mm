#import <Foundation/Foundation.h>
#import <ServiceManagement/ServiceManagement.h>
#import <Cocoa/Cocoa.h>
#include <QDebug>

namespace utils {
namespace mac {

void hideDockIcon(bool hide)
{
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    OSStatus err;
    if (hide)
        err = TransformProcessType(&psn, kProcessTransformToUIElementApplication);
    else
        err = TransformProcessType(&psn, kProcessTransformToForegroundApplication);
}

void orderFrontRegardless(unsigned long long win_id, bool force)
{
  NSView *widget =  (__bridge NSView*)(void*)win_id;
  NSWindow *window = [widget window];
  if(force || [window isVisible])
  [window performSelector:@selector(orderFrontRegardless) withObject:nil afterDelay:0.05];
}

/*
 * On osx if using sandbox, we need to create a helper app to start our app:
 * https://blog.timschroeder.net/2012/07/03/the-launch-at-login-sandbox-project/
 * https://developer.apple.com/library/content/documentation/Security/Conceptual/AppSandboxDesignGuide/AboutAppSandbox/AboutAppSandbox.html
 * https://github.com/olav-st/screencloud/tree/master/ScreenCloudHelper
 *
 * For now we can just add ourself to the login items list in System Preferences / Users/ Login Items
 */

LSSharedFileListItemRef FindLoginItemForCurrentBundle(CFArrayRef currentLoginItems)
{
    CFURLRef mainBundleURL = CFBundleCopyBundleURL(CFBundleGetMainBundle());

    for (int i = 0, end = CFArrayGetCount(currentLoginItems);i < end;++i)
    {
        LSSharedFileListItemRef item = (LSSharedFileListItemRef)CFArrayGetValueAtIndex(currentLoginItems, i);

        UInt32 resolutionFlags = kLSSharedFileListNoUserInteraction | kLSSharedFileListDoNotMountVolumes;
        CFURLRef url = NULL;
        OSStatus err = LSSharedFileListItemResolve(item, resolutionFlags, &url, NULL);

        if (err == noErr)
        {
            bool foundIt = CFEqual(url, mainBundleURL);
            CFRelease(url);

            if (foundIt)
            {
                CFRelease(mainBundleURL);
                return item;
            }
        }
    }

    CFRelease(mainBundleURL);
    return NULL;
}

void setAutoStartup(bool en)
{
    LSSharedFileListRef loginItems = LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);

    if (!loginItems)
    {
        qWarning() << "LSSharedFileListCreate() failed";
        return;
    }

    UInt32 seed = 0U;
    CFArrayRef currentLoginItems = LSSharedFileListCopySnapshot(loginItems, &seed);
    LSSharedFileListItemRef existingItem = FindLoginItemForCurrentBundle(currentLoginItems);

    if (en && (existingItem == NULL))
    {
        CFURLRef mainBundleURL = CFBundleCopyBundleURL(CFBundleGetMainBundle());
        LSSharedFileListInsertItemURL(loginItems, kLSSharedFileListItemBeforeFirst, NULL, NULL, mainBundleURL, NULL, NULL);
        CFRelease(mainBundleURL);
    }
    else if (!en && (existingItem != NULL))
    {
        LSSharedFileListItemRemove(loginItems, existingItem);
    }

    CFRelease(currentLoginItems);
    CFRelease(loginItems);

}

} // namespace mac
} // namespace utils
