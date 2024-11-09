#include "KeyboardLayoutDetector.h"
#include <QJsonObject>
#include "WSClient.h"

KeyboardLayoutDetector::KeyboardLayoutDetector(QObject *parent)
: KeyboardLayoutDetectorOS{parent}
{
    fillKeyboardLayoutMap();
}

/**
 * @brief KeyboardLayoutDetector::setCurrentLayout
 * Fetch current layout and check mapping for layouts available on device,
 * if mapping exists than setting current layout to usb/bt layout.
 */
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

/**
 * @brief KeyboardLayoutDetector::setReceivedLayouts
 * Store received layouts from the device for mapping with OS layout
 * @param layouts Received layouts from the device
 */
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
        m_setLayoutAfterLabelsReceived = false;
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
