#ifndef KEYBOARDLAYOUTDETECTOR_H
#define KEYBOARDLAYOUTDETECTOR_H

#include <QObject>
#include <QString>
#include <QMap>

#if defined(Q_OS_WIN)
#include "KeyboardLayoutDetectorWin.h"
using KeyboardLayoutDetectorOS = KeyboardLayoutDetectorWin;
#elif defined(Q_OS_LINUX)
#include "KeyboardLayoutDetectorUnix.h"
using KeyboardLayoutDetectorOS = KeyboardLayoutDetectorUnix;
#elif defined(Q_OS_MAC)
#include "KeyboardLayoutDetectorMac.h"
using KeyboardLayoutDetectorOS = KeyboardLayoutDetectorMac;
#elif defined(Q_OS_MAC)
#endif

class WSClient;

class KeyboardLayoutDetector : public KeyboardLayoutDetectorOS
{
    Q_OBJECT

public:
    KeyboardLayoutDetector(QObject *parent = nullptr);

    void setCurrentLayout();
    void setReceivedLayouts(const QJsonObject& layouts);
    void reset();
    void setWsClient(WSClient* wsClient);
public slots:
    void onNewDeviceDetected();
private:
    QMap<QString, int> m_deviceLayouts;
    bool m_filled = false;
    bool m_labelsReceived = false;
    bool m_setLayoutAfterLabelsReceived = false;
    WSClient* m_wsClient = nullptr;
};

#endif // KEYBOARDLAYOUTDETECTOR_H
