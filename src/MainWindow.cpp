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

#define CSS_BLUE_BUTTON "QPushButton {" \
                            "color: #fff;" \
                            "background-color: #60B1C7;" \
                            "padding: 15px;" \
                            "border: none;" \
                        "}" \
                        "QPushButton:hover {" \
                            "background-color: #3d96af;" \
                        "}"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    awesome = new QtAwesome(this);
    awesome->initFontAwesome();

    ui->setupUi(this);

    ui->pushButtonDevSettings->setIcon(awesome->icon(fa::wrench));
    ui->pushButtonCred->setIcon(awesome->icon(fa::key));
    ui->pushButtonSync->setIcon(awesome->icon(fa::refresh));

    ui->labelLogo->setPixmap(QPixmap(":/mp-logo.png").scaled(500, ui->widgetHeader->sizeHint().height() - 8, Qt::KeepAspectRatio));

    wsClient = new WSClient(this);
//    connect(wsClient, &WSClient::connectedChanged, [=]()
//    {
//        if (wsClient->get_connected())
//            ui->plainTextEdit->appendPlainText("Mooltipass connected");
//        else
//            ui->plainTextEdit->appendPlainText("Mooltipass disconnected");
//    });
//    connect(wsClient, &WSClient::statusChanged, [=]()
//    {
//        ui->plainTextEdit->appendPlainText(QString("Status: %1").arg(Common::MPStatusString[wsClient->get_status()]));
//    });

    ui->pushButtonMemMode->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonExportFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonImportFile->setStyleSheet(CSS_BLUE_BUTTON);

    connect(ui->pushButtonDevSettings, SIGNAL(clicked(bool)), this, SLOT(updatedPage()));
    connect(ui->pushButtonCred, SIGNAL(clicked(bool)), this, SLOT(updatedPage()));
    connect(ui->pushButtonSync, SIGNAL(clicked(bool)), this, SLOT(updatedPage()));

    ui->pushButtonDevSettings->setChecked(true);
    ui->stackedWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updatedPage()
{
    if (ui->pushButtonDevSettings->isChecked())
        ui->stackedWidget->setCurrentIndex(0);
    else if (ui->pushButtonCred->isChecked())
        ui->stackedWidget->setCurrentIndex(1);
    else if (ui->pushButtonSync->isChecked())
        ui->stackedWidget->setCurrentIndex(2);
}
