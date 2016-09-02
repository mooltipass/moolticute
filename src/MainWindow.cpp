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

    ui->labelLogo->setPixmap(QPixmap(":/mp-logo.png").scaled(500, ui->widgetHeader->sizeHint().height() - 8, Qt::KeepAspectRatio));

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
    ui->pushButtonCredEdit->setStyleSheet(CSS_BLUE_BUTTON);

    ui->pushButtonSettingsSave->setIcon(awesome->icon(fa::floppyo, whiteButtons));
    ui->pushButtonSettingsReset->setIcon(awesome->icon(fa::undo, whiteButtons));
    ui->pushButtonCredAdd->setIcon(awesome->icon(fa::plus, whiteButtons));
    ui->pushButtonCredDel->setIcon(awesome->icon(fa::trash, whiteButtons));
    ui->pushButtonShowPass->setIcon(awesome->icon(fa::eye, whiteButtons));
    ui->pushButtonExitMMM->setIcon(awesome->icon(fa::signout, whiteButtons));
    ui->pushButtonCredEdit->setIcon(awesome->icon(fa::pencilsquareo, whiteButtons));
    ui->pushButtonSettingsSave->setVisible(false);
    ui->pushButtonSettingsReset->setVisible(false);

    connect(ui->pushButtonDevSettings, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonCred, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonSync, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonAbout, SIGNAL(clicked(bool)), this, SLOT(updatePage()));

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

    //When device has new parameters, update the GUI
    connect(wsClient, &WSClient::mpHwVersionChanged, [=]()
    {
        ui->widgetParamMini->setVisible(wsClient->get_mpHwVersion() == Common::MP_Mini);
    });
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
    connect(wsClient, &WSClient::askPasswordDone, [=](bool success, const QString &pass)
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
                passItem->setText(pass);
        }
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

    connect(ui->comboBoxScreenBrightness, SIGNAL(currentIndexChanged(int)), this, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxKnock, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));
    connect(ui->comboBoxKnock, SIGNAL(currentIndexChanged(int)), this, SLOT(checkSettingsChanged()));

    //Setup the confirm view
    ui->widgetSpin->setPixmap(awesome->icon(fa::circleonotch).pixmap(QSize(80, 80)));

    connect(wsClient, SIGNAL(memMgmtModeChanged(bool)), this, SLOT(memMgmtMode()));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    wsClient->closeWebsocket();
    event->accept();
}

void MainWindow::updatePage()
{
    if (ui->pushButtonAbout->isChecked())
    {
        ui->stackedWidget->setCurrentIndex(PAGE_ABOUT);
        return;
    }

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
    {
        if (wsClient->get_memMgmtMode())
            ui->stackedWidget->setCurrentIndex(PAGE_CREDENTIALS);
        else
            ui->stackedWidget->setCurrentIndex(PAGE_CREDENTIALS_ENABLE);
    }
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
    if (ui->checkBoxKnock->isChecked() != wsClient->get_knockEnabled())
        uichanged = true;
    if (ui->comboBoxKnock->currentData().toInt() != wsClient->get_knockSensitivity())
        uichanged = true;
    if (ui->comboBoxScreenBrightness->currentData().toInt() != wsClient->get_screenBrightness())
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

    switch (wsClient->get_screenBrightness())
    {
    default:
    case 20: ui->comboBoxScreenBrightness->setCurrentIndex(0); break;
    case 35: ui->comboBoxScreenBrightness->setCurrentIndex(1); break;
    case 50: ui->comboBoxScreenBrightness->setCurrentIndex(2); break;
    case 65: ui->comboBoxScreenBrightness->setCurrentIndex(3); break;
    case 80: ui->comboBoxScreenBrightness->setCurrentIndex(4); break;
    case 100: ui->comboBoxScreenBrightness->setCurrentIndex(5); break;
    }

    ui->comboBoxKnock->setCurrentIndex(wsClient->get_knockSensitivity());

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
                     { "tutorial_enabled", ui->checkBoxTuto->isChecked() },
                     { "screen_brightness", ui->comboBoxScreenBrightness->currentData().toInt() },
                     { "knock_enabled", ui->checkBoxKnock->isChecked() },
                     { "knock_sensitivity", ui->comboBoxKnock->currentData().toInt() }};

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
    {
        ui->widgetHeader->setEnabled(false);
        ui->pushButtonCred->setChecked(true); //force
        ui->stackedWidget->setCurrentIndex(PAGE_CREDENTIALS);
    }
    else
    {
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
    if (!wsClient->get_memMgmtMode()) return;

    DialogEdit d(credModel);
    if (d.exec())
    {

    }
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

    DialogEdit d(credModel);

    QStandardItem *pit = credModel->item(idx.parent().row());
    d.setService(pit->text());

    QStandardItem *it = pit->child(idx.row(), 1);
    d.setLogin(it->text());

    it = pit->child(idx.row(), 2);
    d.setPassword(it->text());

    it = pit->child(idx.row(), 3);
    d.setDescription(it->text());

    if (d.exec())
    {

    }
}
