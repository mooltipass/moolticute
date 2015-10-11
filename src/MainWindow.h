#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void usbDeviceAdded();
    void usbDeviceRemoved();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
