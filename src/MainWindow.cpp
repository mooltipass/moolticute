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

template <typename T>
static void updateComboBoxIndex(QComboBox* cb, const T & value, int defaultIdx = 0) {
    int idx = cb->findData(QVariant::fromValue(value));
    if(idx < 0)
        idx = defaultIdx;
    cb->setCurrentIndex(idx);
}


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
    ui->credentialsListView->setModel(credFilterModel);

    QDataWidgetMapper* mapper = new QDataWidgetMapper(this);
    mapper->setItemDelegate(new CredentialViewItemDelegate(mapper));
    mapper->setModel(credFilterModel);
    mapper->addMapping(ui->credDisplayServiceInput, CredentialsModel::ServiceIdx);
    mapper->addMapping(ui->credDisplayLoginInput, CredentialsModel::LoginIdx);
    mapper->addMapping(ui->credDisplayPasswordInput, CredentialsModel::PasswordIdx);
    mapper->addMapping(ui->credDisplayDescriptionInput,  CredentialsModel::DescriptionIdx);
    mapper->addMapping(ui->credDisplayCreationDateInput,  CredentialsModel::DateCreatedIdx);
    mapper->addMapping(ui->credDisplayModificationDateInput,  CredentialsModel::DateModifiedIdx);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);


    delete ui->credentialsListView->selectionModel();
    ui->credentialsListView->setSelectionModel(
                new ConditionalItemSelectionModel(ConditionalItemSelectionModel::TestFunction(std::bind(&MainWindow::confirmDiscardUneditedCredentialChanges, this, std::placeholders::_1)),
                                                  credFilterModel));


    connect(ui->credentialsListView->selectionModel(), &QItemSelectionModel::currentRowChanged, mapper, [this, mapper](const QModelIndex & idx) {
        mapper->setCurrentIndex(idx.row());
    });
    connect(ui->credDisplayPasswordInput, & LockedPasswordLineEdit::unlockRequested,
            this, &MainWindow::requestPasswordForSelectedItem);


    ui->credDisplayFrame->setStyleSheet("background-color:#FFFFFF");


    //Disable this option for now, firmware does not support it
    ui->checkBoxInput->setEnabled(false);

    ui->pushButtonDevSettings->setIcon(awesome->icon(fa::wrench));
    ui->pushButtonCred->setIcon(awesome->icon(fa::key));
    ui->pushButtonSync->setIcon(awesome->icon(fa::refresh));
    ui->pushButtonAppSettings->setIcon(awesome->icon(fa::cogs));
    ui->pushButtonAbout->setIcon(awesome->icon(fa::info));

    connect(ui->credDisplayButtonBox, &QDialogButtonBox::clicked, [mapper, this](QAbstractButton* btn) {
        if(ui->credDisplayButtonBox->button(QDialogButtonBox::Save) == btn) {
            saveSelectedCredential();
        }
        if(ui->credDisplayButtonBox->button(QDialogButtonBox::Reset) == btn) {
            mapper->revert();
        }
    });


    ui->labelLogo->setPixmap(QPixmap(":/mp-logo.png").scaledToHeight(ui->widgetHeader->sizeHint().height() - 8, Qt::SmoothTransformation));

    connect(wsClient, &WSClient::wsConnected, this, &MainWindow::updatePage);
    connect(wsClient, &WSClient::wsDisconnected, this, &MainWindow::updatePage);
    connect(wsClient, &WSClient::connectedChanged, this, &MainWindow::updatePage);
    connect(wsClient, &WSClient::statusChanged, this, &MainWindow::updatePage);

    connect(wsClient, &WSClient::statusChanged, [this]() {
        this->enableKnockSettings(wsClient->get_status() == Common::NoCardInserted);
    });

    connect(wsClient, &WSClient::memMgmtModeChanged, this, &MainWindow::enableCredentialsManagement);

    ui->pushButtonMemMode->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonExportFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonImportFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSettingsReset->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSettingsSave->setStyleSheet(CSS_BLUE_BUTTON);

    ui->pushButtonAutoStart->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonViewLogs->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonIntegrity->setStyleSheet(CSS_BLUE_BUTTON);
    ui->addCredentialButton->setStyleSheet(CSS_BLUE_BUTTON);

    ui->pushButtonSettingsSave->setIcon(awesome->icon(fa::floppyo, whiteButtons));
    ui->pushButtonSettingsReset->setIcon(awesome->icon(fa::undo, whiteButtons));
    ui->pushButtonSettingsSave->setVisible(false);
    ui->pushButtonSettingsReset->setVisible(false);
    ui->pushButtonMemMode->setIcon(awesome->icon(fa::lock, whiteButtons));

    connect(ui->pushButtonDevSettings, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonCred, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonSync, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonAbout, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonAppSettings, SIGNAL(clicked(bool)), this, SLOT(updatePage()));


    connect(ui->addCredServiceInput, &QLineEdit::textChanged, this, &MainWindow::updateQuickAddCredentialsButtonState);
    connect(ui->addCredLoginInput, &QLineEdit::textChanged, this, &MainWindow::updateQuickAddCredentialsButtonState);
    connect(ui->addCredPasswordInput, &QLineEdit::textChanged, this, &MainWindow::updateQuickAddCredentialsButtonState);
    updateQuickAddCredentialsButtonState();


    ui->pushButtonDevSettings->setChecked(false);


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


    using LF = Common::LockUnlockModeFeatureFlags;

    ui->lockUnlockModeComboBox->addItem(tr("Disabled")          , (uint)LF::Disabled);
    ui->lockUnlockModeComboBox->addItem(tr("Password Only")     , (uint)LF::Password);
    ui->lockUnlockModeComboBox->addItem(tr("Login + Password")  , (uint)LF::Password|(uint)LF::Login);
    ui->lockUnlockModeComboBox->addItem(tr("Enter + Password")  , (uint)LF::Password|(uint)LF::SendEnter);
    ui->lockUnlockModeComboBox->addItem(tr("Password / Win+L")  , (uint)LF::Password|(uint)LF::SendWin_L);
    ui->lockUnlockModeComboBox->addItem(tr("Login+Pass / Win+L"), (uint)LF::Password|(uint)LF::Login|(uint)LF::SendWin_L);
    ui->lockUnlockModeComboBox->addItem(tr("Enter+Pass / Win+L"), (uint)LF::Password|(uint)LF::SendEnter|(uint)LF::SendWin_L);
    ui->lockUnlockModeComboBox->setCurrentIndex(0);

    //When device has new parameters, update the GUI
    connect(wsClient, &WSClient::mpHwVersionChanged, [=]()
    {
        ui->widgetParamMini->setVisible(wsClient->get_mpHwVersion() == Common::MP_Mini);
        ui->labelAbouHwSerial->setVisible(wsClient->get_mpHwVersion() == Common::MP_Mini);
    });

    connect(wsClient, &WSClient::connectedChanged, this, &MainWindow::updateSerialInfos);
    connect(wsClient, &WSClient::fwVersionChanged, this, &MainWindow::updateSerialInfos);
    connect(wsClient, &WSClient::hwSerialChanged, this, &MainWindow::updateSerialInfos);


    connect(wsClient, &WSClient::hwSerialChanged, [this](quint32 serial) {
         setUIDRequestInstructionsWithId(serial > 0 ? QString::number(serial) : "XXXX");
    });
    connect(wsClient, &WSClient::connectedChanged, [this] () {
        setUIDRequestInstructionsWithId("XXXX");
   });

    QRegularExpressionValidator* uidKeyValidator = new QRegularExpressionValidator(QRegularExpression("[0-9A-Fa-f]{32}"), ui->UIDRequestKeyInput);
    ui->UIDRequestKeyInput->setValidator(uidKeyValidator);
    ui->UIDRequestValidateBtn->setEnabled(false);
    connect(ui->UIDRequestKeyInput, &QLineEdit::textEdited, [this] () {
        ui->UIDRequestValidateBtn->setEnabled(ui->UIDRequestKeyInput->hasAcceptableInput());
    });

    gb_spinner = new QMovie(":/uid_spinner.gif",  {}, this);

    ui->UIDRequestResultLabel->setVisible(false);
    ui->UIDRequestResultIcon->setVisible(false);

    connect(ui->UIDRequestValidateBtn, &QPushButton::clicked, [this]() {
        if(wsClient) {
            wsClient->requestDeviceUID(ui->UIDRequestKeyInput->text().toUtf8());
            ui->UIDRequestResultIcon->setMovie(gb_spinner);
            ui->UIDRequestResultIcon->setVisible(true);
            ui->UIDRequestResultLabel->setVisible(true);
            ui->UIDRequestResultLabel->setText(tr("Fetching UID from device. This may take a few seconds..."));
            ui->UIDRequestResultIcon->setAttribute(Qt::WA_NoSystemBackground);
            gb_spinner->start();
        }
    });

    connect(ui->UIDRequestKeyInput, &QLineEdit::returnPressed, ui->UIDRequestValidateBtn, &QPushButton::click);

    connect(wsClient, &WSClient::uidChanged, [this](qint64 uid) {
        ui->UIDRequestResultLabel->setVisible(true);
        ui->UIDRequestResultIcon->setVisible(true);
        gb_spinner->stop();
        if(uid <= 0) {
            ui->UIDRequestResultIcon->setPixmap(QPixmap(":/message_error.png").scaledToHeight(ui->UIDRequestResultIcon->height(), Qt::SmoothTransformation));
            ui->UIDRequestResultLabel->setText("<span style='color:#FF0000; font-weight:bold'>" + tr("Either the device have been tempered with or the input key is invalid.") + "</span>");
            return;
        }
        ui->UIDRequestResultIcon->setPixmap(QPixmap(":/message_success.png").scaledToHeight(ui->UIDRequestResultIcon->height(), Qt::SmoothTransformation));
        ui->UIDRequestResultLabel->setText("<span style='color:#006400'>" + tr("Your device's UID is %1").arg(QString::number(uid, 16).toUpper()) + "</span>");
    });

    connect(wsClient, &WSClient::connectedChanged, [this](bool connected) {
       ui->UIDRequestGB->setVisible(connected);
       gb_spinner->stop();
       ui->UIDRequestResultLabel->setMovie(nullptr);
       ui->UIDRequestResultLabel->setText({});
       ui->UIDRequestResultLabel->setVisible(false);
       ui->UIDRequestResultIcon->setVisible(false);

    });


    connect(wsClient, &WSClient::keyboardLayoutChanged, [=]()
    {
       updateComboBoxIndex(ui->comboBoxLang, wsClient->get_keyboardLayout());
       checkSettingsChanged();
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
        updateComboBoxIndex(ui->comboBoxScreenBrightness, wsClient->get_screenBrightness());
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

    connect(wsClient, &WSClient::displayHashChanged, [=]()
    {
        ui->hashDisplayFeatureCheckBox->setChecked(wsClient->get_displayHash());
        checkSettingsChanged();
    });

    connect(wsClient, &WSClient::lockUnlockModeChanged, [=]()
    {
        updateComboBoxIndex(ui->lockUnlockModeComboBox, wsClient->get_lockUnlockMode());
    });


    connect(wsClient, &WSClient::keyAfterLoginSendEnableChanged, [=]()
    {
        ui->checkBoxSendAfterLogin->setChecked(wsClient->get_keyAfterLoginSendEnable());
        checkSettingsChanged();
    });

    connect(wsClient, &WSClient::keyAfterLoginSendChanged, [=]()
    {

        updateComboBoxIndex(ui->comboBoxLoginOutput, wsClient->get_keyAfterLoginSend());
        checkSettingsChanged();
    });

    connect(wsClient, &WSClient::keyAfterPassSendEnableChanged, [=]()
    {
        ui->checkBoxSendAfterPassword->setChecked(wsClient->get_keyAfterPassSendEnable());
        checkSettingsChanged();
    });

    connect(wsClient, &WSClient::keyAfterPassSendChanged, [=]()
    {
        updateComboBoxIndex(ui->comboBoxPasswordOutput, wsClient->get_keyAfterPassSend());
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
    });
    connect(ui->lineEditFilterCred, &QLineEdit::textChanged, [=](const QString &t)
    {
        credFilterModel->setFilter(t);
    });

    connect(wsClient, &WSClient::passwordUnlocked, this, &MainWindow::onPasswordUnlocked);
    connect(wsClient, &WSClient::credentialsUpdated, this, &MainWindow::onCredentialUpdated);

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
    connect(ui->hashDisplayFeatureCheckBox, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));
    connect(ui->lockUnlockModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxSendAfterLogin, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));
    connect(ui->comboBoxLoginOutput, SIGNAL(currentIndexChanged(int)), this, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxSendAfterPassword, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));
    connect(ui->comboBoxPasswordOutput, SIGNAL(currentIndexChanged(int)), this, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxSlowHost, SIGNAL(toggled(bool)), this, SLOT(checkSettingsChanged()));
    connect(ui->spinBoxInputDelayAfterKeyPressed, SIGNAL(valueChanged(int)), this, SLOT(checkSettingsChanged()));

    //Setup the confirm view
    ui->widgetSpin->setPixmap(awesome->icon(fa::circleonotch).pixmap(QSize(80, 80)));



    checkAutoStart();

    //Check is ssh agent opt has to be checked
    QSettings s;
    ui->checkBoxSSHAgent->setChecked(s.value("settings/auto_start_ssh").toBool());

    ui->scrollArea->setStyleSheet("QScrollArea { background-color:transparent; }");
    ui->scrollAreaWidgetContents->setStyleSheet("#scrollAreaWidgetContents { background-color:transparent; }");


    updatePage();
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
    if(ui->stackedWidget->currentWidget() == ui->pageCredentials && wsClient->get_memMgmtMode()) {
        if(!confirmDiscardUneditedCredentialChanges())
            return;
        if(QMessageBox::question(this, tr("Exit the credentials manager?"),
                                       tr("Switching tabs will lock out the credentials management mode. Are you sure you want to switch tab ?"),
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
            return;

         wsClient->sendLeaveCredentialsManagementRequest();
    }

    if (ui->pushButtonAbout->isChecked())
    {
        ui->stackedWidget->setCurrentWidget(ui->pageAbout);
        return;
    }

    if (ui->pushButtonAppSettings->isChecked())
    {
        ui->stackedWidget->setCurrentWidget(ui->pageAppSettings);
        return;
    }

    if(!wsClient->isConnected()) {
         ui->stackedWidget->setCurrentWidget(ui->pageNoDaemon);
         return;
    }

    if (!wsClient->get_connected())
    {
        ui->stackedWidget->setCurrentWidget(ui->pageNoConnect);
        return;
    }

    if (ui->pushButtonDevSettings->isChecked()) {
        ui->stackedWidget->setCurrentWidget(ui->pageSettings);
        return;
    }

    if (wsClient->get_status() == Common::NoCardInserted)
    {
        ui->stackedWidget->setCurrentWidget(ui->pageMissingSecurityCard);
        return;
    }

    if (wsClient->get_status() == Common::Locked ||
        wsClient->get_status() == Common::LockedScreen)
    {
        ui->stackedWidget->setCurrentWidget(ui->pageDeviceLocked);
        return;
    }

    else if (ui->pushButtonCred->isChecked())
    {
         ui->stackedWidget->setCurrentWidget(ui->pageCredentials);
    }
    else if (ui->pushButtonSync->isChecked())
        ui->stackedWidget->setCurrentWidget(ui->pageSync);
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
    if (ui->hashDisplayFeatureCheckBox->isChecked() != wsClient->get_displayHash())
        uichanged = true;
    if (ui->lockUnlockModeComboBox->currentData().toInt() != wsClient->get_lockUnlockMode())
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
    ui->hashDisplayFeatureCheckBox->setChecked(wsClient->get_displayHash());
    ui->checkBoxSendAfterLogin->setChecked(wsClient->get_keyAfterLoginSendEnable());
    ui->checkBoxSendAfterPassword->setChecked(wsClient->get_keyAfterPassSendEnable());
    ui->checkBoxSlowHost->setChecked(wsClient->get_delayAfterKeyEntryEnable());
    ui->spinBoxInputDelayAfterKeyPressed->setValue(wsClient->get_delayAfterKeyEntry());

    updateComboBoxIndex(ui->lockUnlockModeComboBox, wsClient->get_lockUnlockMode());
    updateComboBoxIndex(ui->comboBoxLoginOutput, wsClient->get_keyAfterLoginSend());
    updateComboBoxIndex(ui->comboBoxPasswordOutput, wsClient->get_keyAfterPassSend());
    updateComboBoxIndex(ui->comboBoxScreenBrightness, wsClient->get_screenBrightness());
    updateComboBoxIndex(ui->comboBoxLang, wsClient->get_keyboardLayout());

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
    if (ui->hashDisplayFeatureCheckBox->isChecked() != wsClient->get_displayHash())
        o["hash_display"] = ui->hashDisplayFeatureCheckBox->isChecked();
    if (ui->lockUnlockModeComboBox->currentData().toInt() != wsClient->get_lockUnlockMode())
        o["lock_unlock_mode"] = ui->lockUnlockModeComboBox->currentData().toInt();
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
    if(!wsClient->get_memMgmtMode()) {

        wsClient->sendEnterCredentialsManagementRequest();
        ui->stackedWidget->setCurrentWidget(ui->pageWaiting);
    }
    else {
        if(!confirmDiscardUneditedCredentialChanges())
            return;
         wsClient->sendLeaveCredentialsManagementRequest();
    }
}

void MainWindow::enableCredentialsManagement(bool enable)
{
    if(enable && ui->stackedWidget->currentWidget() == ui->pageWaiting)
        ui->stackedWidget->setCurrentWidget(ui->pageCredentials);
    ui->credentialsListView->setEnabled(enable);
    ui->lineEditFilterCred->setEnabled(enable);
    ui->credDisplayFrame ->setVisible(enable);


    if(!enable) {
         updatePage();
    }
    ui->pushButtonMemMode->setIcon(awesome->icon(enable ? fa::lock : fa::unlock));
    ui->pushButtonMemMode->setText(enable ? tr("Lock") : tr("Unlock"));
}


void MainWindow::updateQuickAddCredentialsButtonState() {
    ui->addCredentialButton->setEnabled(   ui->addCredLoginInput->hasAcceptableInput() && ui->addCredLoginInput->text().length() > 0
                                        && ui->addCredServiceInput->hasAcceptableInput() && ui->addCredServiceInput->text().length() > 0
                                        && ui->addCredPasswordInput->hasAcceptableInput() && ui->addCredPasswordInput->text().length() > 0);
}


void MainWindow::requestPasswordForSelectedItem()
{
    if (!wsClient->get_memMgmtMode()) return;

    QItemSelectionModel *selection = ui->credentialsListView->selectionModel();
    QModelIndexList indexes = selection->selectedIndexes();

    if (indexes.size() != 1)
        return;

    QModelIndex idx = credFilterModel->mapToSource(indexes.at(0));

    const auto&  cred = credModel->at(idx.row());
    wsClient->requestPassword(cred.service, cred.login);
}

void MainWindow::on_addCredentialButton_clicked()
{
    ui->addCredentialsGroupBox->setEnabled(false);
    wsClient->addOrUpdateCredential(ui->addCredServiceInput->text(),
                            ui->addCredLoginInput->text(), ui->addCredPasswordInput->text());


    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(wsClient, &WSClient::credentialsUpdated, [this, conn](const QString & service, const QString & login, const QString &, bool success)
    {
        disconnect(*conn);
        ui->addCredentialsGroupBox->setEnabled(true);
        if (!success)
        {
            QMessageBox::warning(this, tr("Failure"), tr("Unable to set credential %1/%2!").arg(service, login));
            return;
        }

        QMessageBox::information(this, tr("Moolticute"), tr("New credential %1/%2 added successfully.").arg(service, login));
        ui->addCredServiceInput->clear();
        ui->addCredLoginInput->clear();
        ui->addCredPasswordInput->clear();
    });
}

void MainWindow::onPasswordUnlocked(const QString & service, const QString & login,
                                    const QString & password, bool success)
{
    if (!success)
    {
        QMessageBox::warning(this, tr("Failure"), tr("Unable to query password!"));
        return;
    }

    credModel->setClearTextPassword(service, login, password);
}

void MainWindow::onCredentialUpdated(const QString & service, const QString & login, const QString & description, bool success) {
    if (!success)
    {
        QMessageBox::warning(this, tr("Failure"), tr("Unable to modify %1/%2").arg(service, login));
        return;
    }

    credModel->update(service, login, description);
}

void  MainWindow::saveSelectedCredential(QModelIndex idx) {
    QItemSelectionModel *selection = ui->credentialsListView->selectionModel();
    if(!idx.isValid()) {
        QModelIndexList indexes = selection->selectedIndexes();

        if (indexes.size() != 1)
            return;

        idx = credFilterModel->mapToSource(indexes.at(0));
    }
    const auto&  cred = credModel->at(idx.row());

    QString password = ui->credDisplayPasswordInput->text();
    QString description = ui->credDisplayDescriptionInput->text();

    if(password != cred.password || description != cred.description) {
        wsClient->addOrUpdateCredential(cred.service, cred.login, password, description);
    }
}

bool MainWindow::confirmDiscardUneditedCredentialChanges(QModelIndex idx) {
    if(ui->stackedWidget->currentWidget() != ui->pageCredentials || !wsClient->get_memMgmtMode())
        return true;


    if(!idx.isValid()) {
        QItemSelectionModel *selection = ui->credentialsListView->selectionModel();
        QModelIndexList indexes = selection->selectedIndexes();
        if (indexes.size() != 1)
            return true;

        idx = credFilterModel->mapToSource(indexes.at(0));
    }
    const auto&  cred = credModel->at(idx.row());

    QString password = ui->credDisplayPasswordInput->text();
    QString description = ui->credDisplayDescriptionInput->text();

    if(password != cred.password || description != cred.description) {
        auto btn = QMessageBox::question(this, tr("Discard Modifications ?"),
                                           tr("You have modified %1/%2 - Do you want to discard the modifications ?").arg(cred.service, cred.login),
                                         QMessageBox::Discard | QMessageBox::Save | QMessageBox::Cancel, QMessageBox::Cancel);
        if(btn == QMessageBox::Cancel)
            return false;
        if(btn == QMessageBox::Discard)
            return true;
        if(btn == QMessageBox::Save) {
            saveSelectedCredential(idx);
            return true;
        }
    }
    return true;
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
        ui->stackedWidget->setCurrentWidget(ui->pageIntegrity);
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
    ui->stackedWidget->setCurrentWidget(ui->pageSync);
    ui->widgetHeader->setEnabled(true);
}

void MainWindow::setUIDRequestInstructionsWithId(const QString & id)
{
    ui->UIDRequestLabel->setText(tr(R"(
                                    To be sure that no one has tempered with your device, you can request a password which will allow you to fetch the UID of your device.<ol>
                                    <li>Get the serial number from the back of your device.</li>
                                    <li>&shy;<a href="mailto:support@themooltipass.com?subject=UID Request Code&body=My serial number is %1">Send us an email</a> with the serial number, requesting the password.</li>
                                    <li>Enter the password you received from us</li></ol>)").arg(id));
}
