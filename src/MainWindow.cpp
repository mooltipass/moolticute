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
                        "}" \
                        "QPushButton:pressed {" \
                            "background-color: #237C95;" \
                        "}"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    awesome = new QtAwesome(this);
    awesome->initFontAwesome();

    ui->setupUi(this);

    //Disable this option for now, firmware does not support it
    ui->checkBoxInput->setEnabled(false);
    ui->spinBoxInput->setEnabled(false);

    ui->pushButtonDevSettings->setIcon(awesome->icon(fa::wrench));
    ui->pushButtonCred->setIcon(awesome->icon(fa::key));
    ui->pushButtonSync->setIcon(awesome->icon(fa::refresh));

    ui->labelLogo->setPixmap(QPixmap(":/mp-logo.png").scaled(500, ui->widgetHeader->sizeHint().height() - 8, Qt::KeepAspectRatio));

    wsClient = new WSClient(this);
    connect(wsClient, &WSClient::connectedChanged, [=]() { updatePage(); });
    connect(wsClient, &WSClient::statusChanged, [=]() { updatePage(); });

    ui->pushButtonMemMode->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonExportFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonImportFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSettingsReset->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSettingsSave->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonCredAdd->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonCredDel->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonShowPass->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonExitMMM->setStyleSheet(CSS_BLUE_BUTTON);

    ui->pushButtonSettingsSave->setIcon(awesome->icon(fa::floppyo, {{ "color", QColor(Qt::white) }}));
    ui->pushButtonSettingsReset->setIcon(awesome->icon(fa::undo, {{ "color", QColor(Qt::white) }}));
    ui->pushButtonCredAdd->setIcon(awesome->icon(fa::plus, {{ "color", QColor(Qt::white) }}));
    ui->pushButtonCredDel->setIcon(awesome->icon(fa::trash, {{ "color", QColor(Qt::white) }}));
    ui->pushButtonShowPass->setIcon(awesome->icon(fa::eye, {{ "color", QColor(Qt::white) }}));
    ui->pushButtonExitMMM->setIcon(awesome->icon(fa::signout, {{ "color", QColor(Qt::white) }}));
    ui->pushButtonSettingsSave->setVisible(false);
    ui->pushButtonSettingsReset->setVisible(false);

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

    //When device has new parameters, update the GUI
    connect(wsClient, &WSClient::keyboardLayoutChanged, [=]()
    {
        for (int i = 0;i < ui->comboBoxLang->count();i++)
        {
            if (ui->comboBoxLang->itemData(i).toInt() == wsClient->get_keyboardLayout())
            {
                ui->comboBoxLang->setCurrentIndex(i);
                checkSettingsChanged();
                break;
            }
        }
    });
    connect(wsClient, &WSClient::lockTimeoutEnabledChanged, [=]()
    {
        ui->checkBoxLock->setChecked(wsClient->get_lockTimeoutEnabled());
        checkSettingsChanged();
    });
    connect(wsClient, &WSClient::lockTimeoutChanged, [=]()
    {
        ui->spinBoxLock->setValue(wsClient->get_lockTimeout());
        checkSettingsChanged();
    });
    connect(wsClient, &WSClient::screensaverChanged, [=]()
    {
        ui->checkBoxScreensaver->setChecked(wsClient->get_screensaver());
        checkSettingsChanged();
    });
    connect(wsClient, &WSClient::userRequestCancelChanged, [=]()
    {
        ui->checkBoxInput->setChecked(wsClient->get_userRequestCancel());
        checkSettingsChanged();
    });
    connect(wsClient, &WSClient::userInteractionTimeoutChanged, [=]()
    {
        ui->spinBoxInput->setValue(wsClient->get_userInteractionTimeout());
        checkSettingsChanged();
    });
    connect(wsClient, &WSClient::flashScreenChanged, [=]()
    {
        ui->checkBoxFlash->setChecked(wsClient->get_flashScreen());
        checkSettingsChanged();
    });
    connect(wsClient, &WSClient::offlineModeChanged, [=]()
    {
        ui->checkBoxBoot->setChecked(wsClient->get_offlineMode());
        checkSettingsChanged();
    });
    connect(wsClient, &WSClient::tutorialEnabledChanged, [=]()
    {
        ui->checkBoxTuto->setChecked(wsClient->get_tutorialEnabled());
        checkSettingsChanged();
    });

    //When something changed in GUI, show save/reset buttons
    connect(ui->comboBoxLang, SIGNAL(currentIndexChanged(int)), this, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxLock, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));
    connect(ui->spinBoxLock, SIGNAL(valueChanged(int)), this, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxScreensaver, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxInput, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));
    connect(ui->spinBoxInput, SIGNAL(valueChanged(int)), this, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxFlash, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxBoot, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxTuto, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));

    //Setup the confirm view
    ui->widgetSpin->setPixmap(awesome->icon(fa::circleonotch).pixmap(QSize(80, 80)));

    connect(wsClient, SIGNAL(memMgmtModeChanged(bool)), this, SLOT(memMgmtMode()));
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

void MainWindow::checkSettingsChanged()
{
    //lang combobox
    bool uichanged = false;
    if (ui->comboBoxLang->currentData().toInt() != wsClient->get_keyboardLayout())
        uichanged = true;
    if (ui->checkBoxLock->isChecked() != wsClient->get_lockTimeoutEnabled())
        uichanged = true;
    if (ui->spinBoxLock->value() != wsClient->get_lockTimeout())
        uichanged = true;
    if (ui->checkBoxScreensaver->isChecked() != wsClient->get_screensaver())
        uichanged = true;
    if (ui->checkBoxInput->isChecked() != wsClient->get_userRequestCancel())
        uichanged = true;
    if (ui->spinBoxInput->value() != wsClient->get_userInteractionTimeout())
        uichanged = true;
    if (ui->checkBoxFlash->isChecked() != wsClient->get_flashScreen())
        uichanged = true;
    if (ui->checkBoxBoot->isChecked() != wsClient->get_offlineMode())
        uichanged = true;
    if (ui->checkBoxTuto->isChecked() != wsClient->get_tutorialEnabled())
        uichanged = true;

    if (uichanged)
    {
        ui->pushButtonSettingsReset->setVisible(true);
        ui->pushButtonSettingsSave->setVisible(true);
    }
    else
    {
        ui->pushButtonSettingsReset->setVisible(false);
        ui->pushButtonSettingsSave->setVisible(false);
    }
}

void MainWindow::on_pushButtonSettingsReset_clicked()
{
    for (int i = 0;i < ui->comboBoxLang->count();i++)
    {
        if (ui->comboBoxLang->itemData(i).toInt() == wsClient->get_keyboardLayout())
        {
            ui->comboBoxLang->setCurrentIndex(i);
            checkSettingsChanged();
            break;
        }
    }
    ui->checkBoxLock->setChecked(wsClient->get_lockTimeoutEnabled());
    ui->spinBoxLock->setValue(wsClient->get_lockTimeout());
    ui->checkBoxScreensaver->setChecked(wsClient->get_screensaver());
    ui->checkBoxInput->setChecked(wsClient->get_userRequestCancel());
    ui->spinBoxInput->setValue(wsClient->get_userInteractionTimeout());
    ui->checkBoxFlash->setChecked(wsClient->get_flashScreen());
    ui->checkBoxBoot->setChecked(wsClient->get_offlineMode());
    ui->checkBoxTuto->setChecked(wsClient->get_tutorialEnabled());

    ui->pushButtonSettingsReset->setVisible(false);
    ui->pushButtonSettingsSave->setVisible(false);
}

void MainWindow::on_pushButtonSettingsSave_clicked()
{
    QJsonObject o = {{ "keyboard_layout", ui->comboBoxLang->currentData().toInt() },
                     { "lock_timeout_enabled", ui->checkBoxLock->isChecked() },
                     { "lock_timeout", ui->spinBoxLock->value() * 60 },
                     { "screensaver", ui->checkBoxScreensaver->isChecked() },
                     { "user_request_cancel", ui->checkBoxInput->isChecked() },
                     { "user_interaction_timeout", ui->spinBoxInput->value() },
                     { "flash_screen", ui->checkBoxFlash->isChecked() },
                     { "offline_mode", ui->checkBoxBoot->isChecked() },
                     { "tutorial_enabled", ui->checkBoxTuto->isChecked() }};

    wsClient->sendJsonData({{ "msg", "param_set" }, { "data", o }});
}

void MainWindow::on_pushButtonMemMode_clicked()
{
    wsClient->sendJsonData({{ "msg", "start_memorymgmt" }});
    ui->widgetHeader->setEnabled(false);
    ui->stackedWidget->setCurrentIndex(PAGE_WAIT_CONFIRM);
}

void MainWindow::memMgmtMode()
{
    qDebug() << "MMM changed";
    if (wsClient->get_memMgmtMode())
        ui->stackedWidget->setCurrentIndex(PAGE_CREDENTIALS);
    else
    {
        updatePage();
        ui->widgetHeader->setEnabled(true);
    }
}

void MainWindow::on_pushButtonExitMMM_clicked()
{
    wsClient->sendJsonData({{ "msg", "exit_memorymgmt" }});
}
