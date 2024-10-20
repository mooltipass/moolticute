#include "KeyboardLayoutDetectorMac.h"
#include "Carbon/Carbon.h"

KeyboardLayoutDetectorMac::KeyboardLayoutDetectorMac(QObject *parent)
    : IKeyboardLayoutDetector{parent}
{

}

void KeyboardLayoutDetectorMac::fillKeyboardLayoutMap()
{
    m_layoutMap = {
        {"com.apple.keylayout.Belgian","Belgian French"},
        {"com.apple.keylayout.Brazilian","Brazil"},
        {"com.apple.keylayout.Brazilian-Pro","Brazil"},
        {"com.apple.keylayout.Canadian","Canada"},
        {"com.apple.keylayout.CanadianFrench-PC","Canada French"},
        {"com.apple.keylayout.Canadian-CSA","Canada French"},
        {"com.apple.keylayout.Colemak","Colemak"},
        {"com.apple.keylayout.Czech","Czech"},
        {"com.apple.keylayout.Czech-QWERTY","Czech QWERTY"},
        {"com.apple.keylayout.Danish","Denmark"},
        {"com.apple.keylayout.Dvorak","Dvorak"},
        {"com.apple.keylayout.DVORAK-QWERTYCMD","Dvorak"},
        {"com.apple.keylayout.Dvorak-Left","Dvorak"},
        {"com.apple.keylayout.Dvorak-Right","Dvorak"},
        {"com.apple.keylayout.French","France"},
        {"com.apple.keylayout.French-PC","France"},
        {"com.apple.keylayout.French-numerical","France"},
        {"com.apple.keylayout.German","Germany"},
        {"com.apple.keylayout.German-DIN-2137","Germany"},
        {"com.apple.keylayout.Hungarian","Hungary"},
        {"com.apple.keylayout.Hungarian-QWERTY","Hungary"},
        {"com.apple.keylayout.Icelandic","Iceland"},
        {"com.apple.keylayout.Italian","Italy"},
        {"com.apple.keylayout.Italian-Pro","Italy"},
        {"com.apple.keylayout.LatinAmerican","Latin America"},
        {"com.apple.keylayout.Dutch","Netherlands"},
        {"com.apple.keylayout.Norwegian","Norway"},
        {"com.apple.keylayout.Polish","Poland"},
        {"com.apple.keylayout.PolishPro","Poland"},
        {"com.apple.keylayout.Portuguese","Portugal"},
        {"com.apple.keylayout.Romanian","Romania"},
        {"com.apple.keylayout.Romanian-Standard","Romania"},
        {"com.apple.keylayout.Slovenian","Slovenia"},
        {"com.apple.keylayout.Spanish","Spain"},
        {"com.apple.keylayout.Spanish-ISO","Spain"},
        {"com.apple.keylayout.Swedish","Sweden & Finland"},
        {"com.apple.keylayout.Swedish-Pro","Sweden & Finland"},
        {"com.apple.keylayout.Finnish","Sweden & Finland"},
        {"com.apple.keylayout.SwissFrench","Swiss French"},
        {"com.apple.keylayout.British","United Kingdom"},
        {"com.apple.keylayout.British-PC","UK Extended"},
        {"com.apple.keylayout.USInternational-PC","US International"},
        {"com.apple.keylayout.US","USA"}
    };
}

QString KeyboardLayoutDetectorMac::getKeyboardLayout()
{
    char layout[128];
    memset(layout, '\0', sizeof(layout));
    TISInputSourceRef source = TISCopyCurrentKeyboardInputSource();
    CFStringRef layoutID = (CFStringRef) TISGetInputSourceProperty(source, kTISPropertyInputSourceID);
    CFStringGetCString(layoutID, layout, sizeof(layout), kCFStringEncodingUTF8);
    return QString{layout};
}
