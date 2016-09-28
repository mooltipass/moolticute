#import <Foundation/Foundation.h>
#import <ServiceManagement/ServiceManagement.h>
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

/*
 * On osx if using sandbox, we need to create a helper app to start our app:
 * https://blog.timschroeder.net/2012/07/03/the-launch-at-login-sandbox-project/
 * https://developer.apple.com/library/content/documentation/Security/Conceptual/AppSandboxDesignGuide/AboutAppSandbox/AboutAppSandbox.html
 * https://github.com/olav-st/screencloud/tree/master/ScreenCloudHelper
 *
void setAutoStartup(bool en)
{
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    NSURL *mainBundleURL = [[NSBundle mainBundle] bundleURL];
    NSURL *url = [mainBundleURL URLByAppendingPathComponent: @"Contents/Library/LoginItems/MoolticuteHelper.app"];
    if (LSRegisterURL((CFURLRef)url, true) != noErr)
    {
        WARNING("LSRegisterURL failed. URL: " + QString([[url absoluteString] UTF8String]));
    }

    NSString *ref = @"org.raoulh.moolticute";
    if (!SMLoginItemSetEnabled((CFStringRef)ref, en))
        qWarning() << "SMLoginItemSetEnabled failed.";
}
*/

} // namespace mac
} // namespace utils
