#include "KeyboardLayoutDetector.h"
#include <QDebug>
#include <QJsonObject>

#if defined(Q_OS_WIN)
#include "windows.h"
#include "winuser.h"
#elif defined(Q_OS_LINUX)
//TODO
#elif defined(Q_OS_MAC)
//TODO
#include "Carbon/Carbon.h"
#endif

#include "DeviceConnectionChecker.h"
#include "WSClient.h"

KeyboardLayoutDetector::KeyboardLayoutDetector(QObject *parent)
: QObject{parent}
{
    fillKeyboardLayoutMap();
}

QString KeyboardLayoutDetector::getKeyboardLayout()
{
#if defined(Q_OS_WIN)
    WCHAR wide_layout_name[KL_NAMELENGTH * 2];
    GetKeyboardLayoutNameW(wide_layout_name);
    std::wstring ws(wide_layout_name);
    return QString::fromStdWString(ws);
#elif defined(Q_OS_LINUX)
    // TODO
    return QString{};
#elif defined(Q_OS_MAC)
    char layout[128];
    memset(layout, '\0', sizeof(layout));
    TISInputSourceRef source = TISCopyCurrentKeyboardInputSource();
    CFStringRef layoutID = (CFStringRef) TISGetInputSourceProperty(source, kTISPropertyInputSourceID);
    CFStringGetCString(layoutID, layout, sizeof(layout), kCFStringEncodingUTF8);
    return QString{layout};
#endif
}

void KeyboardLayoutDetector::fillKeyboardLayoutMap()
{
#if defined(Q_OS_WIN)
    m_layoutMap = {
        {"0000080C","Belgian French"},
        {"00000416","Brazil"},
        {"00010416","Brazil"},
        {"00011009","Canada"},
        {"00001009","Canada French"},
        {"00000C0C","Canada French"},
        {"A0000409","Colemak"},
        {"00000405","Czech"},
        {"00010405","Czech QWERTY"},
        {"00000406","Denmark"},
        {"00010409","Dvorak"},
        {"00030409","Dvorak"},
        {"00040409","Dvorak"},
        {"0000040C","France"},
        {"00000407","Germany"},
        {"00010407","Germany"},
        {"0000040E","Hungary"},
        {"0001040E","Hungary"},
        {"0000040F","Iceland"},
        {"00000410","Italy"},
        {"00010410","Italy"},
        {"0000080A","Latin America"},
        {"00000413","Netherlands"},
        {"00000414","Norway"},
        {"00000415","Poland"},
        {"00010415","Poland"},
        {"00000816","Portugal"},
        {"00000418","Romania"},
        {"00010418","Romania"},
        {"00020418","Romania"},
        {"00000424","Slovenia"},
        {"0000040A","Spain"},
        {"0001040A","Spain"},
        {"0002083B","Sweden & Finland"},
        {"0000100C","Swiss French"},
        {"00000809","United Kingdom"},
        {"00000452","UK Extended"},
        {"00020409","US International"},
        {"00000409","USA"}
    };
#elif defined(Q_OS_LINUX)
    // TODO
    return QString{};
#elif defined(Q_OS_MAC)
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
#endif
}

void KeyboardLayoutDetector::setCurrentLayout()
{
    QString layout = getKeyboardLayout();
    if (m_layoutMap.contains(layout))
    {
        const auto layoutName = m_layoutMap[layout];
        if (m_deviceLayouts.contains(layoutName))
        {
            const auto layoutId = m_deviceLayouts[layoutName];
            if (m_wsClient != nullptr)
            {
                m_wsClient->sendChangedParam("keyboard_usb_layout", layoutId);
                m_wsClient->sendChangedParam("keyboard_bt_layout", layoutId);
            }
            else
            {
                qCritical() << "WSClient is not available, layout is not set.";
            }
        }
        else
        {
            qWarning() << "There is no device layout for: " << layoutName << ". Not setting layout based on detected keyboard setting.";
        }
    }
    else
    {
        qWarning() << "There is no mapping for layout: " << layout << ". Not setting layout based on detected keyboard setting.";
    }

}

void KeyboardLayoutDetector::setReceivedLayouts(const QJsonObject &layouts)
{
    for (auto it = layouts.begin(); it != layouts.end(); ++it)
    {
        m_deviceLayouts.insert(it.key(), it.value().toInt());
    }

    m_labelsReceived = true;

    if (m_setLayoutAfterLabelsReceived)
    {
        setCurrentLayout();
    }
}

void KeyboardLayoutDetector::reset()
{
    m_labelsReceived = false;
    m_setLayoutAfterLabelsReceived = false;
    m_deviceLayouts.clear();
}

void KeyboardLayoutDetector::setWsClient(WSClient *wsClient)
{
    m_wsClient = wsClient;
}

void KeyboardLayoutDetector::onNewDeviceDetected()
{
    if (m_labelsReceived)
    {
        setCurrentLayout();
    }
    else
    {
        // Labels are not fetched from device yet, need to wait it
        m_setLayoutAfterLabelsReceived = true;
    }
}
