#ifndef USBMONITOR_LINUX_H
#define USBMONITOR_LINUX_H

#include <QtCore>
#include <libusb.h>

class UsbMonitor_linux: public QObject
{
    Q_OBJECT
public:
    static UsbMonitor_linux *Instance()
    {
        static UsbMonitor_linux inst;
        return &inst;
    }
    ~UsbMonitor_linux();

signals:
    void usbDeviceAdded(const QUuid &guid, const QString &dev_path);
    void usbDeviceRemoved(const QUuid &guid, const QString &dev_path);

private:
    UsbMonitor_linux();

    libusb_device_handle *handle = NULL;
};

#endif // USBMONITOR_LINUX_H
