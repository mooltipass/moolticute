#ifndef USBMONITOR_MAC_H
#define USBMONITOR_MAC_H

#include <QtCore>

class UsbMonitor_mac: public QObject
{
    Q_OBJECT
public:
    static UsbMonitor_mac *Instance()
    {
        static UsbMonitor_mac inst;
        return &inst;
    }
    ~UsbMonitor_mac();

signals:
    void usbDeviceAdded();
    void usbDeviceRemoved();

private:
    UsbMonitor_mac();
};

#endif // USBMONITOR_MAC_H
