/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Calaos is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Calaos is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets>
#include "WSClient.h"
#include <QtAwesome.h>

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
    void updatePage();
    void checkSettingsChanged();

//    void mpAdded(MPDevice *device);
//    void mpRemoved(MPDevice *);

    void on_pushButtonSettingsReset_clicked();

    void on_pushButtonSettingsSave_clicked();

private:
    Ui::MainWindow *ui;
    QtAwesome* awesome;

    WSClient *wsClient;

    enum
    {
        PAGE_NO_CONNECTION = 0,
        PAGE_SETTINGS = 1,
        PAGE_CREDENTIALS_ENABLE = 2,
        PAGE_SYNC = 3,
        PAGE_NO_CARD = 4,
        PAGE_LOCKED = 5,
        PAGE_CREDENTIALS = 100,
    };
};

#endif // MAINWINDOW_H
