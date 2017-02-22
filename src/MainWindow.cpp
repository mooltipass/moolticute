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
#include "DialogEdit.h"
#include "version.h"
#include "AutoStartup.h"
#include "Common.h"

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

MainWindow::MainWindow(WSClient *client, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    wsClient(client)
{
    awesome = new QtAwesome(this);
    awesome->initFontAwesome();

    QVariantMap whiteButtons = {{ "color", QColor(Qt::white) },
                                { "color-selected", QColor(Qt::white) },
                                { "color-active", QColor(Qt::white) }};

    ui->setupUi(this);

    ui->labelAboutVers->setText(ui->labelAboutVers->text().arg(APP_VERSION));

    credModel = new CredentialsModel(this);
    credFilterModel = new CredentialsFilterModel(this);

    credFilterModel->setSourceModel(credModel);
    ui->treeViewCred->setModel(credFilterModel);

    //Disable this option for now, firmware does not support it
    ui->checkBoxInput->setEnabled(false);

    ui->pushButtonDevSettings->setIcon(awesome->icon(fa::wrench));
    ui->pushButtonCred->setIcon(awesome->icon(fa::key));
    ui->pushButtonSync->setIcon(awesome->icon(fa::refresh));
    ui->pushButtonAppSettings->setIcon(awesome->icon(fa::cogs));
    ui->pushButtonAbout->setIcon(awesome->icon(fa::info));

    ui->labelLogo->setPixmap(QPixmap(":/mp-logo.png").scaledToHeight(ui->widgetHeader->sizeHint().height() - 8, Qt::SmoothTransformation));

    connect(wsClient, &WSClient::connectedChanged, this, &MainWindow::updatePage);
    connect(wsClient, &WSClient::statusChanged, this, &MainWindow::updatePage);

    connect(wsClient, &WSClient::statusChanged, [this]() {
        this->enableKnockSettings(wsClient->get_status() == Common::NoCardInserted);
    });

    ui->pushButtonMemMode->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonExportFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonImportFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSettingsReset->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSettingsSave->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonCredAdd->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonCredDel->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonShowPass->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonExitMMM->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonCredEdit->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonQuickAddCred->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonAutoStart->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonViewLogs->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonIntegrity->setStyleSheet(CSS_BLUE_BUTTON);

    ui->pushButtonSettingsSave->setIcon(awesome->icon(fa::floppyo, whiteButtons));
    ui->pushButtonSettingsReset->setIcon(awesome->icon(fa::undo, whiteButtons));
    ui->pushButtonCredAdd->setIcon(awesome->icon(fa::plus, whiteButtons));
    ui->pushButtonCredDel->setIcon(awesome->icon(fa::trash, whiteButtons));
    ui->pushButtonShowPass->setIcon(awesome->icon(fa::eye, whiteButtons));
    ui->pushButtonExitMMM->setIcon(awesome->icon(fa::signout, whiteButtons));
    ui->pushButtonCredEdit->setIcon(awesome->icon(fa::pencilsquareo, whiteButtons));
    ui->pushButtonSettingsSave->setVisible(false);
    ui->pushButtonSettingsReset->setVisible(false);
    ui->pushButtonMemMode->setIcon(awesome->icon(fa::database, whiteButtons));
    ui->pushButtonQuickAddCred->setIcon(awesome->icon(fa::plussquare, whiteButtons));

    connect(ui->pushButtonDevSettings, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonCred, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonSync, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonAbout, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonAppSettings, SIGNAL(clicked(bool)), this, SLOT(updatePage()));

    ui->pushButtonDevSettings->setChecked(false);
    ui->stackedWidget->setCurrentIndex(PAGE_NO_CONNECTION);

    //Add languages to combobox
    ui->comboBoxLang->addItem("en_US", ID_KEYB_EN_US_LUT);
    ui->comboBoxLang->addItem("fr_FR", ID_KEYB_FR_FR_LUT);
    ui->comboBoxLang->addItem("es_ES", ID_KEYB_ES_ES_LUT);
    ui->comboBoxLang->addItem("de_DE", ID_KEYB_DE_DE_LUT);
    ui->comboBoxLang->addItem("es_AR", ID_KEYB_ES_AR_LUT);
    ui->comboBoxLang->addItem("en_AU", ID_KEYB_EN_AU_LUT);
    ui->comboBoxLang->addItem("fr_BE", ID_KEYB_FR_BE_LUT);
    ui->comboBoxLang->addItem("po_BR", ID_KEYB_PO_BR_LUT);
    ui->comboBoxLang->addItem("en_CA", ID_KEYB_EN_CA_LUT);
    ui->comboBoxLang->addItem("cz_CZ", ID_KEYB_CZ_CZ_LUT);
    ui->comboBoxLang->addItem("da_DK", ID_KEYB_DA_DK_LUT);
    ui->comboBoxLang->addItem("fi_FI", ID_KEYB_FI_FI_LUT);
    ui->comboBoxLang->addItem("hu_HU", ID_KEYB_HU_HU_LUT);
    ui->comboBoxLang->addItem("is_IS", ID_KEYB_IS_IS_LUT);
    ui->comboBoxLang->addItem("it_IT", ID_KEYB_IT_IT_LUT);
    ui->comboBoxLang->addItem("nl_NL", ID_KEYB_NL_NL_LUT);
    ui->comboBoxLang->addItem("no_NO", ID_KEYB_NO_NO_LUT);
    ui->comboBoxLang->addItem("po_PO", ID_KEYB_PO_PO_LUT);
    ui->comboBoxLang->addItem("ro_RO", ID_KEYB_RO_RO_LUT);
    ui->comboBoxLang->addItem("sl_SL", ID_KEYB_SL_SL_LUT);
    ui->comboBoxLang->addItem("frde_CH", ID_KEYB_FRDE_CH_LUT);
    ui->comboBoxLang->addItem("en_UK", ID_KEYB_EN_UK_LUT);
    ui->comboBoxLang->addItem("cs_QWERTY", ID_KEYB_CZ_QWERTY_LUT);
    ui->comboBoxLang->addItem("en_DB", ID_KEYB_EN_DV_LUT);
    ui->comboBoxLang->addItem("fr_MAC", ID_KEYB_FR_MAC_LUT);
    ui->comboBoxLang->addItem("fr_CH_MAC", ID_KEYB_FR_CH_MAC_LUT);
    ui->comboBoxLang->addItem("de_CH_MAC", ID_KEYB_DE_CH_MAC_LUT);
    ui->comboBoxLang->addItem("de_MAC", ID_KEYB_DE_MAC_LUT);
    ui->comboBoxLang->addItem("us_MAC", ID_KEYB_US_MAC_LUT);

    QSortFilterProxyModel* proxy = new QSortFilterProxyModel(ui->comboBoxLang);
    proxy->setSourceModel( ui->comboBoxLang->model());
    ui->comboBoxLang->model()->setParent(proxy);
    ui->comboBoxLang->setModel(proxy);
    ui->comboBoxLang->model()->sort(0);

    ui->widgetParamMini->setVisible(false);
    ui->comboBoxScreenBrightness->addItem("20%", 51);
    ui->comboBoxScreenBrightness->addItem("35%", 89);
    ui->comboBoxScreenBrightness->addItem("50%", 128);
    ui->comboBoxScreenBrightness->addItem("65%", 166);
    ui->comboBoxScreenBrightness->addItem("80%", 204);
    ui->comboBoxScreenBrightness->addItem("100%", 255);
    ui->comboBoxKnock->addItem("Low", 0);
    ui->comboBoxKnock->addItem("Medium", 1);
    ui->comboBoxKnock->addItem("High", 2);
    ui->comboBoxLoginOutput->addItem(tr("Tab"), 43);
    ui->comboBoxLoginOutput->addItem(tr("Enter"), 40);
    ui->comboBoxLoginOutput->addItem(tr("Space"), 44);
    ui->comboBoxPasswordOutput->addItem(tr("Tab"), 43);
    ui->comboBoxPasswordOutput->addItem(tr("Enter"), 40);
    ui->comboBoxPasswordOutput->addItem(tr("Space"), 44);

    //When device has new parameters, update the GUI
    connect(wsClient, &WSClient::mpHwVersionChanged, [=]()
    {
        ui->widgetParamMini->setVisible(wsClient->get_mpHwVersion() == Common::MP_Mini);
        ui->labelAbouHwSerial->setVisible(wsClient->get_mpHwVersion() == Common::MP_Mini);
    });

    connect(wsClient, &WSClient::connectedChanged, this, &MainWindow::updateSerialInfos);
    connect(wsClient, &WSClient::fwVersionChanged, this, &MainWindow::updateSerialInfos);
    connect(wsClient, &WSClient::hwSerialChanged, this, &MainWindow::updateSerialInfos);


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
    connect(wsClient, &WSClient::screenBrightnessChanged, [=]()
    {
        switch (wsClient->get_screenBrightness())
        {
        default:
        case 51: ui->comboBoxScreenBrightness->setCurrentIndex(0); break;
        case 89: ui->comboBoxScreenBrightness->setCurrentIndex(1); break;
        case 128: ui->comboBoxScreenBrightness->setCurrentIndex(2); break;
        case 166: ui->comboBoxScreenBrightness->setCurrentIndex(3); break;
        case 204: ui->comboBoxScreenBrightness->setCurrentIndex(4); break;
        case 255: ui->comboBoxScreenBrightness->setCurrentIndex(5); break;
        }
        checkSettingsChanged();
    });
    connect(wsClient, &WSClient::knockEnabledChanged, [=]()
    {
        ui->checkBoxKnock->setChecked(wsClient->get_knockEnabled());
        checkSettingsChanged();
    });
    connect(wsClient, &WSClient::knockSensitivityChanged, [=]()
    {
        ui->comboBoxKnock->setCurrentIndex(wsClient->get_knockSensitivity());
        checkSettingsChanged();
    });
    connect(wsClient, &WSClient::randomStartingPinChanged, [=]()
    {
        ui->randomStartingPinCheckBox->setChecked(wsClient->get_randomStartingPin());
        checkSettingsChanged();
    });

    connect(wsClient, &WSClient::keyAfterLoginSendEnableChanged, [=]()
    {
        ui->checkBoxSendAfterLogin->setChecked(wsClient->get_keyAfterLoginSendEnable());
        checkSettingsChanged();
    });

    connect(wsClient, &WSClient::keyAfterLoginSendChanged, [=]()
    {
        switch (wsClient->get_keyAfterLoginSend())
        {
        default:
        case 43: ui->comboBoxLoginOutput->setCurrentIndex(0); break;
        case 40: ui->comboBoxLoginOutput->setCurrentIndex(1); break;
        case 44: ui->comboBoxLoginOutput->setCurrentIndex(2); break;
        }
        checkSettingsChanged();
    });

    connect(wsClient, &WSClient::keyAfterPassSendEnableChanged, [=]()
    {
        ui->checkBoxSendAfterPassword->setChecked(wsClient->get_keyAfterPassSendEnable());
        checkSettingsChanged();
    });

    connect(wsClient, &WSClient::keyAfterPassSendChanged, [=]()
    {
        switch (wsClient->get_keyAfterPassSend())
        {
        default:
        case 43: ui->comboBoxPasswordOutput->setCurrentIndex(0); break;
        case 40: ui->comboBoxPasswordOutput->setCurrentIndex(1); break;
        case 44: ui->comboBoxPasswordOutput->setCurrentIndex(2); break;
        }
        checkSettingsChanged();
    });

    connect(wsClient, &WSClient::delayAfterKeyEntryEnableChanged, [=]()
    {
        ui->checkBoxSlowHost->setChecked(wsClient->get_delayAfterKeyEntryEnable());
        checkSettingsChanged();
    });

    connect(wsClient, &WSClient::delayAfterKeyEntryChanged, [=]()
    {
        ui->spinBoxInputDelayAfterKeyPressed->setValue(wsClient->get_delayAfterKeyEntry());
        checkSettingsChanged();
    });

    connect(wsClient, &WSClient::memoryDataChanged, [=]()
    {
        credModel->load(wsClient->getMemoryData()["login_nodes"].toArray());
        ui->lineEditFilterCred->clear();
        ui->treeViewCred->expandAll();
        for (int i = 0;i < credModel->columnCount();i++)
            ui->treeViewCred->resizeColumnToContents(i);
    });
    connect(ui->lineEditFilterCred, &QLineEdit::textChanged, [=](const QString &t)
    {
        credFilterModel->setFilter(t);
    });

    connect(wsClient, &WSClient::askPasswordDone, this, &MainWindow::askPasswordDone);

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

    connect(ui->comboBoxScreenBrightness, SIGNAL(currentIndexChanged(int)), this, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxKnock, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));
    connect(ui->comboBoxKnock, SIGNAL(currentIndexChanged(int)), this, SLOT(checkSettingsChanged()));
    connect(ui->randomStartingPinCheckBox, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxSendAfterLogin, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));
    connect(ui->comboBoxLoginOutput, SIGNAL(currentIndexChanged(int)), this, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxSendAfterPassword, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));
    connect(ui->comboBoxPasswordOutput, SIGNAL(currentIndexChanged(int)), this, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxSlowHost, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));
    connect(ui->spinBoxInputDelayAfterKeyPressed, SIGNAL(valueChanged(int)), this, SLOT(checkSettingsChanged()));

    //Setup the confirm view
    ui->widgetSpin->setPixmap(awesome->icon(fa::circleonotch).pixmap(QSize(80, 80)));

    connect(wsClient, SIGNAL(memMgmtModeChanged(bool)), this, SLOT(memMgmtMode()));

    checkAutoStart();

    //Check is ssh agent opt has to be checked
    QSettings s;
    ui->checkBoxSSHAgent->setChecked(s.value("settings/auto_start_ssh").toBool());

    ui->scrollArea->setStyleSheet("QScrollArea { background-color:transparent; }");
    ui->scrollAreaWidgetContents->setStyleSheet("#scrollAreaWidgetContents { background-color:transparent; }");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
    emit windowCloseRequested();
}

void MainWindow::updatePage()
{
    if (ui->pushButtonAbout->isChecked())
    {
        ui->stackedWidget->setCurrentIndex(PAGE_ABOUT);
        return;
    }

    if (ui->pushButtonAppSettings->isChecked())
    {
        ui->stackedWidget->setCurrentIndex(PAGE_MC_SETTINGS);
        return;
    }

    if(!wsClient->isConnected()) {
         ui->stackedWidget->setCurrentIndex(PAGE_NO_DAEMON);
         return;
    }

    if (!wsClient->get_connected())
    {
        ui->stackedWidget->setCurrentIndex(PAGE_NO_CONNECTION);
        return;
    }

    if (ui->pushButtonDevSettings->isChecked()) {
        ui->stackedWidget->setCurrentIndex(PAGE_SETTINGS);
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

    else if (ui->pushButtonCred->isChecked())
    {
        if (wsClient->get_memMgmtMode())
            ui->stackedWidget->setCurrentIndex(PAGE_CREDENTIALS);
        else
            ui->stackedWidget->setCurrentIndex(PAGE_CREDENTIALS_ENABLE);
    }
    else if (ui->pushButtonSync->isChecked())
        ui->stackedWidget->setCurrentIndex(PAGE_SYNC);
}

void MainWindow::enableKnockSettings(bool enable)
{
    ui->knockSettingsFrame->setEnabled(enable);

    ui->knockSettingsFrame->setToolTip(enable ? "" : tr("Remove the card from the device to change this setting."));
    ui->knockSettingsFrame->setToolTipDuration(enable ? -1 : std::numeric_limits<int>::max());

    //Make sure the suffix label ("sensitivity") matches the color of the other widgets.
    const QString color =
            ui->checkBoxKnock->palette().color(enable ?
                                               QPalette::Active : QPalette::Disabled, QPalette::ButtonText).name();

    ui->knockSettingsSuffixLabel->setStyleSheet(QStringLiteral("color: %1") .arg(color));
}

void MainWindow::updateSerialInfos() {
    ui->labelAbouHwSerial->setText(tr("Device Serial: %1").arg(wsClient->get_hwSerial()));
    ui->labelAboutFwVers->setText(QStringLiteral("Device Firmware version: %1").arg(wsClient->get_fwVersion()));

    const bool connected = wsClient->get_connected();

    ui->labelAboutFwVers->setVisible(connected);
    ui->labelAbouHwSerial->setVisible(connected && wsClient->get_hwSerial() > 0);
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
    if (ui->checkBoxKnock->isChecked() != wsClient->get_knockEnabled())
        uichanged = true;
    if (ui->comboBoxKnock->currentData().toInt() != wsClient->get_knockSensitivity())
        uichanged = true;
    if (ui->randomStartingPinCheckBox->isChecked() != wsClient->get_randomStartingPin())
        uichanged = true;
    if (ui->comboBoxScreenBrightness->currentData().toInt() != wsClient->get_screenBrightness())
        uichanged = true;
    if (ui->checkBoxSendAfterLogin->isChecked() != wsClient->get_keyAfterLoginSendEnable())
        uichanged = true;
    if (ui->comboBoxLoginOutput->currentData().toInt() != wsClient->get_keyAfterLoginSend())
        uichanged = true;
    if (ui->checkBoxSendAfterPassword->isChecked() != wsClient->get_keyAfterPassSendEnable())
        uichanged = true;
    if (ui->comboBoxPasswordOutput->currentData().toInt() != wsClient->get_keyAfterPassSend())
        uichanged = true;
    if (ui->checkBoxSlowHost->isChecked() != wsClient->get_delayAfterKeyEntryEnable())
        uichanged = true;
    if (ui->spinBoxInputDelayAfterKeyPressed->value() != wsClient->get_delayAfterKeyEntry())
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
    ui->checkBoxKnock->setChecked(wsClient->get_knockEnabled());
    ui->randomStartingPinCheckBox->setChecked(wsClient->get_randomStartingPin());
    ui->checkBoxSendAfterLogin->setChecked(wsClient->get_keyAfterLoginSendEnable());
    ui->checkBoxSendAfterPassword->setChecked(wsClient->get_keyAfterPassSendEnable());
    ui->checkBoxSlowHost->setChecked(wsClient->get_delayAfterKeyEntryEnable());
    ui->spinBoxInputDelayAfterKeyPressed->setValue(wsClient->get_delayAfterKeyEntry());

    switch (wsClient->get_keyAfterLoginSend())
    {
    default:
    case 43: ui->comboBoxLoginOutput->setCurrentIndex(0); break;
    case 40: ui->comboBoxLoginOutput->setCurrentIndex(1); break;
    case 44: ui->comboBoxLoginOutput->setCurrentIndex(2); break;
    }

    switch (wsClient->get_keyAfterPassSend())
    {
    default:
    case 43: ui->comboBoxPasswordOutput->setCurrentIndex(0); break;
    case 40: ui->comboBoxPasswordOutput->setCurrentIndex(1); break;
    case 44: ui->comboBoxPasswordOutput->setCurrentIndex(2); break;
    }

    switch (wsClient->get_screenBrightness())
    {
    default:
    case 51: ui->comboBoxScreenBrightness->setCurrentIndex(0); break;
    case 89: ui->comboBoxScreenBrightness->setCurrentIndex(1); break;
    case 128: ui->comboBoxScreenBrightness->setCurrentIndex(2); break;
    case 166: ui->comboBoxScreenBrightness->setCurrentIndex(3); break;
    case 204: ui->comboBoxScreenBrightness->setCurrentIndex(4); break;
    case 255: ui->comboBoxScreenBrightness->setCurrentIndex(5); break;
    }

    ui->comboBoxKnock->setCurrentIndex(wsClient->get_knockSensitivity());

    ui->pushButtonSettingsReset->setVisible(false);
    ui->pushButtonSettingsSave->setVisible(false);
}

void MainWindow::on_pushButtonSettingsSave_clicked()
{
    QJsonObject o;

    if (ui->comboBoxLang->currentData().toInt() != wsClient->get_keyboardLayout())
        o["keyboard_layout"] = ui->comboBoxLang->currentData().toInt();
    if (ui->checkBoxLock->isChecked() != wsClient->get_lockTimeoutEnabled())
        o["lock_timeout_enabled"] = ui->checkBoxLock->isChecked();
    if (ui->spinBoxLock->value() != wsClient->get_lockTimeout())
        o["lock_timeout"] = ui->spinBoxLock->value();
    if (ui->checkBoxScreensaver->isChecked() != wsClient->get_screensaver())
        o["screensaver"] = ui->checkBoxScreensaver->isChecked();
    if (ui->checkBoxInput->isChecked() != wsClient->get_userRequestCancel())
        o["user_request_cancel"] = ui->checkBoxInput->isChecked();
    if (ui->spinBoxInput->value() != wsClient->get_userInteractionTimeout())
        o["user_interaction_timeout"] = ui->spinBoxInput->value();
    if (ui->checkBoxFlash->isChecked() != wsClient->get_flashScreen())
        o["flash_screen"] = ui->checkBoxFlash->isChecked();
    if (ui->checkBoxBoot->isChecked() != wsClient->get_offlineMode())
        o["offline_mode"] = ui->checkBoxBoot->isChecked();
    if (ui->checkBoxTuto->isChecked() != wsClient->get_tutorialEnabled())
        o["tutorial_enabled"] = ui->checkBoxTuto->isChecked();
    if (ui->comboBoxScreenBrightness->currentData().toInt() != wsClient->get_screenBrightness())
        o["screen_brightness"] = ui->comboBoxScreenBrightness->currentData().toInt();
    if (ui->checkBoxSendAfterLogin->isChecked() != wsClient->get_keyAfterLoginSendEnable())
        o["key_after_login_enabled"] = ui->checkBoxSendAfterLogin->isChecked();
    if (ui->comboBoxLoginOutput->currentData().toInt() != wsClient->get_keyAfterLoginSend())
        o["key_after_login"] = ui->comboBoxLoginOutput->currentData().toInt();
    if (ui->checkBoxSendAfterPassword->isChecked() != wsClient->get_keyAfterPassSendEnable())
        o["key_after_pass_enabled"] = ui->checkBoxSendAfterPassword->isChecked();
    if (ui->comboBoxPasswordOutput->currentData().toInt() != wsClient->get_keyAfterPassSend())
        o["key_after_pass"] = ui->comboBoxPasswordOutput->currentData().toInt();
    if (ui->checkBoxSlowHost->isChecked() != wsClient->get_delayAfterKeyEntryEnable())
        o["delay_after_key_enabled"] = ui->checkBoxSlowHost->isChecked();
    if (ui->randomStartingPinCheckBox->isChecked() != wsClient->get_randomStartingPin())
        o["random_starting_pin"] = ui->randomStartingPinCheckBox->isChecked();
    if (ui->spinBoxInputDelayAfterKeyPressed->value() != wsClient->get_delayAfterKeyEntry())
        o["delay_after_key"] = ui->spinBoxInputDelayAfterKeyPressed->value();


    if(wsClient->get_status() == Common::NoCardInserted) {
        if (ui->checkBoxKnock->isChecked() != wsClient->get_knockEnabled())
            o["knock_enabled"] = ui->checkBoxKnock->isChecked();
        if (ui->comboBoxKnock->currentData().toInt() != wsClient->get_knockSensitivity())
            o["knock_sensitivity"] = ui->comboBoxKnock->currentData().toInt();
    }

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
    if (wsClient->get_memMgmtMode())
    {
        qDebug() << "MMM entered";
        ui->widgetHeader->setEnabled(false);
        ui->pushButtonCred->setChecked(true); //force
        ui->stackedWidget->setCurrentIndex(PAGE_CREDENTIALS);
    }
    else
    {
        qDebug() << "MMM exitted";
        updatePage();
        ui->widgetHeader->setEnabled(true);
        passItem = nullptr;
    }
}

void MainWindow::on_pushButtonExitMMM_clicked()
{
    wsClient->sendJsonData({{ "msg", "exit_memorymgmt" }});
}

void MainWindow::on_pushButtonShowPass_clicked()
{
    if (!wsClient->get_memMgmtMode()) return;

    QItemSelectionModel *selection = ui->treeViewCred->selectionModel();
    QModelIndexList indexes = selection->selectedIndexes();

    if (indexes.size() < 1)
        return;

    QModelIndex idx = credFilterModel->mapToSource(indexes.at(0));

    if (!idx.parent().isValid())
        return;

    QStandardItem *pit = credModel->item(idx.parent().row());
    QString service = pit->text();

    QStandardItem *it = pit->child(idx.row(), 1);
    QString login = it->text();

    passItem = pit->child(idx.row(), 2);

    QJsonObject d = {{ "service", service },
                     { "login", login }};
    wsClient->sendJsonData({{ "msg", "ask_password" },
                            { "data", d }});

    setEnabled(false);
}

void MainWindow::on_pushButtonCredAdd_clicked()
{
    on_pushButtonQuickAddCred_clicked();
}

void MainWindow::on_pushButtonCredEdit_clicked()
{
    if (!wsClient->get_memMgmtMode()) return;

    QItemSelectionModel *selection = ui->treeViewCred->selectionModel();
    QModelIndexList indexes = selection->selectedIndexes();

    if (indexes.size() < 1)
        return;

    QModelIndex idx = credFilterModel->mapToSource(indexes.at(0));

    if (!idx.parent().isValid())
        return;

    QStandardItem *serviceIt = credModel->item(idx.parent().row());
    QStandardItem *loginIt = serviceIt->child(idx.row(), 1);
    QStandardItem *passIt = serviceIt->child(idx.row(), 2);
    QStandardItem *descIt = serviceIt->child(idx.row(), 3);

    //Password is unknown, ask first
    if (!passIt->data(CredentialsModel::RoleHasPassword).toBool())
    {
        on_pushButtonShowPass_clicked();
        editCredAsked = true;
        return;
    }

    DialogEdit d(credModel);
    d.setService(serviceIt->text());
    d.setLogin(loginIt->text());
    d.setPassword(passIt->text());
    d.setDescription(descIt->text());

    if (d.exec())
    {
        setEnabled(false);

        QJsonObject o = {{ "service", d.getService() },
                         { "login", d.getLogin() },
                         { "password", d.getPassword() },
                         { "description", d.getDescription() }};
        wsClient->sendJsonData({{ "msg", "set_credential" },
                                { "data", o }});

        auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = connect(wsClient, &WSClient::addCredentialDone, [=](bool success)
        {
            disconnect(*conn);
            setEnabled(true);
            if (!success)
            {
                QMessageBox::warning(this, tr("Failure"), tr("Unable to set credential!"));
                return;
            }

            serviceIt->setText(o["sevice"].toString());
            loginIt->setText(o["login"].toString());
            passIt->setText(o["password"].toString());
            descIt->setText(o["description"].toString());

            QMessageBox::information(this, tr("Moolticute"), tr("Update of credential done successfully."));
        });
    }
}

void MainWindow::on_pushButtonQuickAddCred_clicked()
{
    DialogEdit d(credModel);
    if (d.exec())
    {
        setEnabled(false);

        QJsonObject o = {{ "service", d.getService() },
                         { "login", d.getLogin() },
                         { "password", d.getPassword() },
                         { "description", d.getDescription() }};
        wsClient->sendJsonData({{ "msg", "set_credential" },
                                { "data", o }});

        auto conn = std::make_shared<QMetaObject::Connection>();
        *conn = connect(wsClient, &WSClient::addCredentialDone, [this, conn](bool success)
        {
            disconnect(*conn);
            setEnabled(true);
            if (!success)
            {
                QMessageBox::warning(this, tr("Failure"), tr("Unable to set credential!"));
                return;
            }

            QMessageBox::information(this, tr("Moolticute"), tr("New credential added successfully."));
        });
    }
}

void MainWindow::askPasswordDone(bool success, const QString &pass)
{
    setEnabled(true);
    if (!success)
    {
        QMessageBox::warning(this, tr("Failure"), tr("Unable to query password!"));
        passItem = nullptr;
    }
    else
    {
        if (passItem)
        {
            passItem->setText(pass);
            passItem->setData(true, CredentialsModel::RoleHasPassword);
            if (editCredAsked)
                QTimer::singleShot(1, this, SLOT(on_pushButtonCredEdit_clicked()));
        }
    }
    editCredAsked = false;
}

void MainWindow::on_pushButtonAutoStart_clicked()
{
    QSettings s;

    bool en = s.value("settings/auto_start").toBool();

    int ret;
    if (en)
        ret = QMessageBox::question(this, "Moolticute", tr("Disable autostart at boot?"));
    else
        ret = QMessageBox::question(this, "Moolticute", tr("Enable autostart at boot?"));

    if (ret == QMessageBox::Yes)
    {
        s.setValue("settings/auto_start", !en);
        s.sync();

        checkAutoStart();
    }
}

void MainWindow::checkAutoStart()
{
    QSettings s;

    bool en = s.value("settings/auto_start").toBool();

    AutoStartup::enableAutoStartup(en);
    ui->labelAutoStart->setText(tr("Start Moolticute with the computer: %1").arg((en?"Enabled":"Disabled")));
    if (en)
        ui->pushButtonAutoStart->setText(tr("Disable"));
    else
        ui->pushButtonAutoStart->setText(tr("Enable"));
}

void MainWindow::daemonLogAppend(const QByteArray &logdata)
{
    if (dialogLog)
        dialogLog->appendData(logdata);
    logBuffer.append(logdata);

    const int maxsz = 100 * 1024;

    if (logBuffer.size() > maxsz)
        logBuffer = logBuffer.right(maxsz);
}

void MainWindow::on_pushButtonViewLogs_clicked()
{
    if (dialogLog)
    {
        dialogLog->show();
        return;
    }

    dialogLog = new WindowLog();
    dialogLog->appendData(logBuffer);
    dialogLog->show();
    connect(dialogLog, &WindowLog::destroyed, [this]()
    {
        dialogLog = nullptr;
    });
}

bool MainWindow::isHttpDebugChecked()
{
    return ui->checkBoxDebugHttp->isChecked();
}

void MainWindow::on_checkBoxSSHAgent_stateChanged(int)
{
    QSettings s;
    s.setValue("settings/auto_start_ssh", ui->checkBoxSSHAgent->isChecked());
}

void MainWindow::on_pushButtonExportFile_clicked()
{
    QMessageBox::information(this, "Moolticute", "Not yet implemented.");
}

void MainWindow::on_pushButtonImportFile_clicked()
{
    QMessageBox::information(this, "Moolticute", "Not yet implemented.");
}

void MainWindow::on_pushButtonIntegrity_clicked()
{
    int r = QMessageBox::question(this, "Moolticute", tr("Do you want to start the integrity check of your device?"));
    if (r == QMessageBox::Yes)
    {
        wsClient->sendJsonData({{ "msg", "start_memcheck" }});
        ui->widgetHeader->setEnabled(false);
        ui->stackedWidget->setCurrentIndex(PAGE_MC_INTEGRITY_CHECK);
        ui->progressBarIntegrity->setMinimum(0);
        ui->progressBarIntegrity->setMaximum(0);
        ui->progressBarIntegrity->setValue(0);

        connect(wsClient, SIGNAL(memcheckFinished(bool)), this, SLOT(integrityFinished(bool)));
        connect(wsClient, SIGNAL(progressChanged(int,int)), this, SLOT(integrityProgress(int,int)));
    }
}

void MainWindow::integrityProgress(int total, int current)
{
    ui->progressBarIntegrity->setMaximum(total);
    ui->progressBarIntegrity->setValue(current);
}

void MainWindow::integrityFinished(bool success)
{
    disconnect(wsClient, SIGNAL(memcheckFinished(bool)), this, SLOT(integrityFinished(bool)));
    disconnect(wsClient, SIGNAL(progressChanged(int,int)), this, SLOT(integrityProgress(int,int)));

    if (!success)
        QMessageBox::warning(this, "Moolticute", tr("Memory integrity check failed!"));
    else
        QMessageBox::information(this, "Moolticute", "Memory integrity check done successfully");
    ui->stackedWidget->setCurrentIndex(PAGE_SYNC);
    ui->widgetHeader->setEnabled(true);
}
