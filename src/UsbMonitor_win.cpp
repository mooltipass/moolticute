#include "UsbMonitor_win.h"

UsbMonitor_win::UsbMonitor_win():
    QWidget()
{
    qDebug() << "Registering device events";

    DEV_BROADCAST_DEVICEINTERFACE db =
    {
        sizeof(DEV_BROADCAST_DEVICEINTERFACE),
        DBT_DEVTYP_DEVICEINTERFACE
    };

    notifyHandle = RegisterDeviceNotification((HWND)winId(),
                                              &db,
                                              DEVICE_NOTIFY_WINDOW_HANDLE | DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
    if (!notifyHandle)
        qWarning() << "Unable to register for device notifications";
}

UsbMonitor_win::~UsbMonitor_win()
{
    if (!UnregisterDeviceNotification(notifyHandle))
        qWarning() << "Unable to unregister for device notifications";
}

bool UsbMonitor_win::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(result);

    MSG *msg = reinterpret_cast<MSG *>(message);
    if (!msg) return false;

    if (msg->message == WM_DEVICECHANGE && msg->lParam)
    {
        DEV_BROADCAST_DEVICEINTERFACE* db = reinterpret_cast<DEV_BROADCAST_DEVICEINTERFACE*>(msg->lParam);

        QString dev_path = QString::fromWCharArray(db->dbcc_name);
        QUuid guid(db->dbcc_classguid);

        if (msg->wParam == DBT_DEVICEARRIVAL)
        {
            qDebug() << "Device added: " << guid.toString() << " - " << dev_path;
            emit usbDeviceAdded(/*guid, dev_path*/);
        }
        else if (msg->wParam == DBT_DEVICEREMOVECOMPLETE)
        {
            qDebug() << "Device removed: " << guid.toString() << " - " << dev_path;
            emit usbDeviceRemoved(/*guid, dev_path*/);
        }
    }

    return false;
}
