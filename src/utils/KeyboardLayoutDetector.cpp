#include "KeyboardLayoutDetector.h"
#include <QDebug>
#include <QJsonObject>
#include "windows.h"
#include "winuser.h"

#include "DeviceConnectionChecker.h"
#include "WSClient.h"

KeyboardLayoutDetector::KeyboardLayoutDetector(QObject *parent)
: QObject{parent}
{
    fillKeyboardLayoutMap();
}

QString KeyboardLayoutDetector::getKeyboardLayout()
{
    WCHAR wide_layout_name[KL_NAMELENGTH * 2];
    GetKeyboardLayoutNameW(wide_layout_name);
    std::wstring ws(wide_layout_name);
    return QString::fromStdWString(ws);
}

void KeyboardLayoutDetector::fillKeyboardLayoutMap()
{
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
