#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <libusb.h>

#ifdef Q_OS_WIN
#include "UsbMonitor_win.h"
#else
#include "UsbMonitor_linux.h"
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    int err;

    err = libusb_init(nullptr);
    if (err < 0)
        QMessageBox::warning(this, tr("Error"), tr("Failed to initialise libusb:\n%1").arg(libusb_error_name(err)));

    if (!libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG))
        qDebug() << "libusb Hotplug capabilites are not supported on this platform";

#ifdef Q_OS_WIN
    connect(UsbMonitor_win::Instance(), SIGNAL(usbDeviceAdded()), this, SLOT(usbDeviceAdded()));
    connect(UsbMonitor_win::Instance(), SIGNAL(usbDeviceRemoved()), this, SLOT(usbDeviceRemoved()));
#else
    connect(UsbMonitor_linux::Instance(), SIGNAL(usbDeviceAdded()), this, SLOT(usbDeviceAdded()));
    connect(UsbMonitor_linux::Instance(), SIGNAL(usbDeviceRemoved()), this, SLOT(usbDeviceRemoved()));
#endif
}

MainWindow::~MainWindow()
{
    UsbMonitor_linux::Instance()->stop();
    libusb_exit(nullptr);
    delete ui;
}

void MainWindow::usbDeviceAdded()
{
    ui->plainTextEdit->appendPlainText("New usb device");
}

void MainWindow::usbDeviceRemoved()
{
    ui->plainTextEdit->appendPlainText("Removed usb device");
}
