#include <Foundation/Foundation.h>

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

} // namespace mac
} // namespace utils
