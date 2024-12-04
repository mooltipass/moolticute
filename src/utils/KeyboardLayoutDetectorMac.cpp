#include "KeyboardLayoutDetectorMac.h"
#include "Carbon/Carbon.h"

KeyboardLayoutDetectorMac::KeyboardLayoutDetectorMac(QObject *parent)
    : IKeyboardLayoutDetector{parent}
{

}

/**
 * @brief KeyboardLayoutDetectorMac::fillKeyboardLayoutMap
 * Fills the mapping for Mac keyboard layout ids with layout names on BLE device
 */
void KeyboardLayoutDetectorMac::fillKeyboardLayoutMap()
{
    m_layoutMap = {
        {"com.apple.keylayout.Belgian","Belgian (MacOS)"},
        {"com.apple.keylayout.Brazilian","Brazil (MacOS)"},
        {"com.apple.keylayout.Brazilian-Pro","Brazil (MacOS)"},
        {"com.apple.keylayout.Canadian","Canada (MacOS)"},
        {"com.apple.keylayout.CanadianFrench-PC","Canada French"},
        {"com.apple.keylayout.Canadian-CSA","Canada French"},
        {"com.apple.keylayout.Colemak","Colemak (MacOS)"},
        {"com.apple.keylayout.Czech","Czech (MacOS)"},
        {"com.apple.keylayout.Czech-QWERTY","Czech QWERTY (MacOS)"},
        {"com.apple.keylayout.Danish","Denmark (MacOS)"},
        {"com.apple.keylayout.Dvorak","Dvorak (MacOS)"},
        {"com.apple.keylayout.DVORAK-QWERTYCMD","Dvorak (MacOS)"},
        {"com.apple.keylayout.Dvorak-Left","Dvorak (MacOS)"},
        {"com.apple.keylayout.Dvorak-Right","Dvorak (MacOS)"},
        {"com.apple.keylayout.French","France (MacOS)"},
        {"com.apple.keylayout.French-PC","France (MacOS)"},
        {"com.apple.keylayout.French-numerical","France (MacOS)"},
        {"com.apple.keylayout.German","Germany (MacOS)"},
        {"com.apple.keylayout.German-DIN-2137","Germany (MacOS)"},
        {"com.apple.keylayout.Hungarian","Hungary (MacOS)"},
        {"com.apple.keylayout.Hungarian-QWERTY","Hungary (MacOS)"},
        {"com.apple.keylayout.Icelandic","Iceland (MacOS)"},
        {"com.apple.keylayout.Italian","Italy (MacOS)"},
        {"com.apple.keylayout.Italian-Pro","Italy (MacOS)"},
        {"com.apple.keylayout.LatinAmerican","Latin America"},
        {"com.apple.keylayout.Dutch","Nertherlands (Mac)"},
        {"com.apple.keylayout.Norwegian","Norway (MacOS)"},
        {"com.apple.keylayout.Polish","Poland (MacOS)"},
        {"com.apple.keylayout.PolishPro","Poland (MacOS)"},
        {"com.apple.keylayout.Portuguese","Portugal (MacOS)"},
        {"com.apple.keylayout.Romanian","Romania (MacOS)"},
        {"com.apple.keylayout.Romanian-Standard","Romania (MacOS)"},
        {"com.apple.keylayout.Slovenian","Slovenia (MacOS)"},
        {"com.apple.keylayout.Spanish","Spain (MacOS)"},
        {"com.apple.keylayout.Spanish-ISO","Spain (MacOS)"},
        {"com.apple.keylayout.Swedish","Sweden/Fin (MacOS)"},
        {"com.apple.keylayout.Swedish-Pro","Sweden/Fin (MacOS)"},
        {"com.apple.keylayout.Finnish","Sweden/Fin (MacOS)"},
        {"com.apple.keylayout.SwissFrench","Swiss FR (MacOS)"},
        {"com.apple.keylayout.British","UK (MacOS)"},
        {"com.apple.keylayout.British-PC","UK (MacOS)"},
        {"com.apple.keylayout.USInternational-PC","US International"},
        {"com.apple.keylayout.US","USA (MacOS)"}
    };
}

/**
 * @brief KeyboardLayoutDetectorMac::getKeyboardLayout
 * @return MacOS keyboard layout id (QString)
 */
QString KeyboardLayoutDetectorMac::getKeyboardLayout()
{
    char layout[128];
    memset(layout, '\0', sizeof(layout));
    TISInputSourceRef source = TISCopyCurrentKeyboardInputSource();
    CFStringRef layoutID = (CFStringRef) TISGetInputSourceProperty(source, kTISPropertyInputSourceID);
    CFStringGetCString(layoutID, layout, sizeof(layout), kCFStringEncodingUTF8);
    return QString{layout};
}
