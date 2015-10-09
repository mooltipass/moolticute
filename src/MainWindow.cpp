#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <libusb.h>

#ifdef Q_OS_WIN
#include "UsbMonitor_win.h"
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    int err;

    err = libusb_init(nullptr);
    if (!err < 0)
        QMessageBox::warning(this, tr("Error"), tr("Failed to initialise libusb:\n%1").arg(libusb_error_name(err)));

    if (!libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG))
        QMessageBox::warning(this, tr("Error"), tr("Hotplug capabilites are not supported on this platform"));

#ifdef Q_OS_WIN
    connect(UsbMonitor_win::Instance(), &UsbMonitor_win::usbDeviceAdded, [=](const QUuid &guid, const QString &dev_path)
    {
        ui->plainTextEdit->appendPlainText(QString("New usb device: %1 - %2").arg(guid.toString()).arg(dev_path));
    });
    connect(UsbMonitor_win::Instance(), &UsbMonitor_win::usbDeviceRemoved, [=](const QUuid &guid, const QString &dev_path)
    {
        ui->plainTextEdit->appendPlainText(QString("Removed usb device: %1 - %2").arg(guid.toString()).arg(dev_path));
    });
#endif
}

MainWindow::~MainWindow()
{
    libusb_exit(nullptr);
    delete ui;
}
