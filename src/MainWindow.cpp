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
    connect(wsClient, &WSClient::connectedChanged, [=]()
    {
        updatePage();
    });
    connect(wsClient, &WSClient::statusChanged, [=]()
    {
        updatePage();
        //ui->plainTextEdit->appendPlainText(QString("Status: %1").arg(Common::MPStatusString[wsClient->get_status()]));
    });

    ui->pushButtonMemMode->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonExportFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonImportFile->setStyleSheet(CSS_BLUE_BUTTON);

    connect(ui->pushButtonDevSettings, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonCred, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonSync, SIGNAL(clicked(bool)), this, SLOT(updatePage()));

    ui->pushButtonDevSettings->setChecked(false);
    ui->stackedWidget->setCurrentIndex(PAGE_NO_CONNECTION);

    //Add languages to combobox
    ui->comboBoxLang->addItem("cz_CZ", 155);
    ui->comboBoxLang->addItem("da_DK", 156);
    ui->comboBoxLang->addItem("de_DE", 149);
    ui->comboBoxLang->addItem("en_AU", 151);
    ui->comboBoxLang->addItem("en_CA", 154);
    ui->comboBoxLang->addItem("en_UK", 167);
    ui->comboBoxLang->addItem("en_US", 146);
    ui->comboBoxLang->addItem("es_AR", 150);
    ui->comboBoxLang->addItem("es_ES", 148);
    ui->comboBoxLang->addItem("fi_FI", 157);
    ui->comboBoxLang->addItem("fr_BE", 152);
    ui->comboBoxLang->addItem("fr_FR", 147);
    ui->comboBoxLang->addItem("frde_CH", 166);
    ui->comboBoxLang->addItem("hu_HU", 158);
    ui->comboBoxLang->addItem("is_IS", 159);
    ui->comboBoxLang->addItem("it_IT", 160);
    ui->comboBoxLang->addItem("nl_NL", 161);
    ui->comboBoxLang->addItem("no_NO", 162);
    ui->comboBoxLang->addItem("po_BR", 153);
    ui->comboBoxLang->addItem("po_PO", 163);
    ui->comboBoxLang->addItem("ro_RO", 164);
    ui->comboBoxLang->addItem("sl_SL", 165);

    connect(wsClient, &WSClient::keyboardLayoutChanged, [=]()
    {
        for (int i = 0;i < ui->comboBoxLang->count();i++)
        {
            if (ui->comboBoxLang->itemData(i).toInt() == wsClient->get_keyboardLayout())
            {
                ui->comboBoxLang->setCurrentIndex(i);
                break;
            }
        }
    });
    connect(wsClient, &WSClient::lockTimeoutEnabledChanged, [=]()
    {
        ui->checkBoxLock->setChecked(wsClient->get_lockTimeoutEnabled());
    });
    connect(wsClient, &WSClient::lockTimeoutChanged, [=]()
    {
        ui->spinBoxLock->setValue(wsClient->get_lockTimeout());
    });
    connect(wsClient, &WSClient::screensaverChanged, [=]()
    {
        ui->checkBoxScreensaver->setChecked(wsClient->get_screensaver());
    });
    connect(wsClient, &WSClient::userRequestCancelChanged, [=]()
    {
        ui->checkBoxInput->setChecked(wsClient->get_userRequestCancel());
    });
    connect(wsClient, &WSClient::userInteractionTimeoutChanged, [=]()
    {
        ui->spinBoxInput->setValue(wsClient->get_userInteractionTimeout());
    });
    connect(wsClient, &WSClient::flashScreenChanged, [=]()
    {
        ui->checkBoxFlash->setChecked(wsClient->get_flashScreen());
    });
    connect(wsClient, &WSClient::offlineModeChanged, [=]()
    {
        ui->checkBoxBoot->setChecked(wsClient->get_offlineMode());
    });
    connect(wsClient, &WSClient::tutorialEnabledChanged, [=]()
    {
        ui->checkBoxTuto->setChecked(wsClient->get_tutorialEnabled());
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updatePage()
{
    if (!wsClient->get_connected())
    {
        ui->stackedWidget->setCurrentIndex(PAGE_NO_CONNECTION);
        return;
    }

    if (wsClient->get_status() == Common::NoCardInserted)
    {
        ui->stackedWidget->setCurrentIndex(PAGE_NO_CARD);
        return;
    }

    if (wsClient->get_status() == Common::Locked ||
        wsClient->get_status() == Common::LockedScreen)
    {
        ui->stackedWidget->setCurrentIndex(PAGE_LOCKED);
        return;
    }

    if (ui->pushButtonDevSettings->isChecked())
        ui->stackedWidget->setCurrentIndex(PAGE_SETTINGS);
    else if (ui->pushButtonCred->isChecked())
        ui->stackedWidget->setCurrentIndex(PAGE_CREDENTIALS_ENABLE);
    else if (ui->pushButtonSync->isChecked())
        ui->stackedWidget->setCurrentIndex(PAGE_SYNC);
}
