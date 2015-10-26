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
#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    wsClient = new WSClient(this);
    connect(wsClient, &WSClient::connectedChanged, [=]()
    {
        if (wsClient->get_connected())
            ui->plainTextEdit->appendPlainText("Mooltipass connected");
        else
            ui->plainTextEdit->appendPlainText("Mooltipass disconnected");
    });
    connect(wsClient, &WSClient::statusChanged, [=]()
    {
        ui->plainTextEdit->appendPlainText(QString("Status: %1").arg(Common::MPStatusString[wsClient->get_status()]));
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
