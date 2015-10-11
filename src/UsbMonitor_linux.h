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

    void stop();

signals:
    void usbDeviceAdded();
    void usbDeviceRemoved();

private:
    UsbMonitor_linux();

    libusb_device_handle *handle = nullptr;
    libusb_hotplug_callback_handle cbaddhandle;
    libusb_hotplug_callback_handle cbdelhandle;

    QMutex mutex;
    bool run = true;

    friend int libusb_device_add_cb(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data);
    friend int libusb_device_del_cb(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data);
};

#endif // USBMONITOR_LINUX_H
