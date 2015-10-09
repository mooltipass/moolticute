#ifndef USBMONITOR_WIN_H
#define USBMONITOR_WIN_H

#include <QtCore>
#include <QWidget>
#include <qt_windows.h>
#include <dbt.h>
#include <windows.h>

class UsbMonitor_win: public QWidget
{
    Q_OBJECT
public:
    static UsbMonitor_win *Instance()
    {
        static UsbMonitor_win inst;
        return &inst;
    }
    ~UsbMonitor_win();

signals:
    void usbDeviceAdded(const QUuid &guid, const QString &dev_path);
    void usbDeviceRemoved(const QUuid &guid, const QString &dev_path);

private:
    UsbMonitor_win();

    HDEVNOTIFY notifyHandle = nullptr;

    virtual bool nativeEvent(const QByteArray &eventType, void *message, long *result);
};

#endif // USBMONITOR_WIN_H
