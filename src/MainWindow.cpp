/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Moolticute is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Moolticute is distributed in the hope that it will be useful,
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
#include "AppGui.h"
#include "PasswordProfilesModel.h"
#include "PassGenerationProfilesDialog.h"
#include "PromptWidget.h"

#include "qtcsv/stringdata.h"
#include "qtcsv/reader.h"

template <typename T>
static void updateComboBoxIndex(QComboBox* cb, const T & value, int defaultIdx = 0)
{
    int idx = cb->findData(QVariant::fromValue(value));
    if (idx < 0)
        idx = defaultIdx;
    cb->setCurrentIndex(idx);
}

void MainWindow::initHelpLabels()
{
    auto getFontAwesomeIconPixmap = [=](int character, QSize size = QSize(20, 20))
    {
        QVariantMap gray = {{ "color", QColor(Qt::gray) },
                            { "color-selected", QColor(Qt::gray) },
                            { "color-active", QColor(Qt::gray) }};

        return AppGui::qtAwesome()->icon(character, gray).pixmap(size);
    };

    ui->label_exportDBHelp->setPixmap(getFontAwesomeIconPixmap(fa::infocircle));
    ui->label_exportDBHelp->setToolTip(tr("All your logins and passwords are stored inside an encrypted database on your Mooltipass device.\r\nTogether with your smartcard, this feature allows you to securely export your user's database to your computer to later import it on other devices."));

    ui->label_importDBHelp->setPixmap(getFontAwesomeIconPixmap(fa::infocircle));
    ui->label_importDBHelp->setToolTip(tr("All your logins and passwords are stored inside an encrypted database on your Mooltipass device.\r\nTogether with your smartcard, this feature allows you to import a database from another Mooltipass into this device."));

    ui->label_integrityCheckHelp->setPixmap(getFontAwesomeIconPixmap(fa::infocircle));
    ui->label_integrityCheckHelp->setToolTip(tr("Only if instructed by the Mooltipass team should you click that button!"));

    ui->label_dbBackupMonitoringHelp->setPixmap(getFontAwesomeIconPixmap(fa::infocircle));
    ui->label_dbBackupMonitoringHelp->setToolTip(tr("Select a backup file to make sure your Mooltipass database is always in sync with it.\r\nYou will be prompted for import or export operations if any changes to your Mooltipass database or monitored file are detected."));

    ui->label_MooltiAppHelp->setPixmap(getFontAwesomeIconPixmap(fa::infocircle));
    ui->label_MooltiAppHelp->setToolTip(tr("The MooltiApp backup file doesn't have encrypted usernames."));

    ui->label_resetCardHelp->setPixmap(getFontAwesomeIconPixmap(fa::infocircle));
    ui->label_resetCardHelp->setToolTip(tr("When an unknown card message is displayed that means you have no database for this user in your Mooltipass device.\nHowever you or other users may have a backup file or may use this card in another device.\nThink twice before resetting a card."));

    ui->label_ImportCSVHelp->setPixmap(getFontAwesomeIconPixmap(fa::infocircle));
    ui->label_ImportCSVHelp->setToolTip(tr("Import unencrypted passwords from comma-separated values text file."));

    ui->label_ExportCSVHelp->setPixmap(getFontAwesomeIconPixmap(fa::infocircle));
    ui->label_ExportCSVHelp->setToolTip(tr("Export unencrypted passwords to comma-separated values text file."));
}

MainWindow::MainWindow(WSClient *client, DbMasterController *mc, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    wsClient(client),
    bSSHKeyTabVisible(false),
    bAdvancedTabVisible(false),
    dbBackupTrakingControlsVisible(false),
    previousWidget(nullptr),
    m_passwordProfilesModel(new PasswordProfilesModel(this)),
    dbMasterController(mc)
{
    QSettings s;
    bSSHKeysTabVisibleOnDemand = s.value("settings/SSHKeysTabsVisibleOnDemand", true).toBool();

    QVariantMap whiteButtons = {{ "color", QColor(Qt::white) },
                                { "color-selected", QColor(Qt::white) },
                                { "color-active", QColor(Qt::white) }};

    ui->setupUi(this);
    refreshAppLangCb();

    ui->checkBoxLongPress->setChecked(s.value("settings/long_press_cancel", true).toBool());
    connect(ui->checkBoxLongPress, &QCheckBox::toggled, [](bool checked)
    {
        QSettings settings;
        settings.setValue("settings/long_press_cancel", checked);
    });

    m_tabMap[ui->pageAbout] = ui->pushButtonAbout;
    m_tabMap[ui->pageAppSettings] = ui->pushButtonAppSettings;
    m_tabMap[ui->pageSettings] = ui->pushButtonDevSettings;
    m_tabMap[ui->pageAdvanced] = ui->pushButtonAdvanced;
    m_tabMap[ui->pageCredentials] = ui->pushButtonCred;
    m_tabMap[ui->pageSync] = ui->pushButtonSync;
    m_tabMap[ui->pageFiles] = ui->pushButtonFiles;
    m_tabMap[ui->pageSSH] = ui->pushButtonSSH;
    m_tabMap[ui->pageBleDev] = ui->pushButtonBleDev;
    connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);

    ui->widgetCredentials->setWsClient(wsClient);
    ui->widgetFiles->setWsClient(wsClient);
    ui->widgetSSH->setWsClient(wsClient);
    ui->widgetBleDev->setWsClient(wsClient);

    ui->widgetCredentials->setPasswordProfilesModel(m_passwordProfilesModel);

    ui->labelAboutVers->setText(ui->labelAboutVers->text().arg(APP_VERSION));

    ui->labelAboutAuxMCU->setText(tr("Aux MCU version:"));
    ui->labelAboutMainMCU->setText(tr("Main MCU version:"));

    initHelpLabels();

    //Disable this option for now, firmware does not support it
    ui->checkBoxInput->setEnabled(false);

    ui->radioButtonSSHTabAlways->setChecked(!bSSHKeysTabVisibleOnDemand);
    ui->radioButtonSSHTabOnDemand->setChecked(bSSHKeysTabVisibleOnDemand);

    ui->pushButtonDevSettings->setIcon(AppGui::qtAwesome()->icon(fa::wrench));
    ui->pushButtonCred->setIcon(AppGui::qtAwesome()->icon(fa::key));
    ui->pushButtonSync->setIcon(AppGui::qtAwesome()->icon(fa::refresh));
    ui->pushButtonAppSettings->setIcon(AppGui::qtAwesome()->icon(fa::cogs));
    ui->pushButtonAbout->setIcon(AppGui::qtAwesome()->icon(fa::info));
    ui->pushButtonFiles->setIcon(AppGui::qtAwesome()->icon(fa::file));

    ui->pushButtonSSH->setIcon(AppGui::qtAwesome()->icon(fa::key));
    ui->pushButtonSSH->setVisible(!bSSHKeysTabVisibleOnDemand || bSSHKeyTabVisible);

    ui->labelLogo->setPixmap(QPixmap(":/mp-logo.png").scaledToHeight(ui->widgetHeader->sizeHint().height() - 8, Qt::SmoothTransformation));
    ui->pushButtonAdvanced->setVisible(bAdvancedTabVisible);

    m_FilesAndSSHKeysTabsShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_F1), this);
    setKeysTabVisibleOnDemand(bSSHKeysTabVisibleOnDemand);
    connect(ui->radioButtonSSHTabAlways, &QRadioButton::toggled, this, &MainWindow::onRadioButtonSSHTabsAlwaysToggled);
    m_advancedTabShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_F2), this);
    connect(m_advancedTabShortcut, SIGNAL(activated()), this, SLOT(onAdvancedTabShortcutActivated()));
    m_BleDevTabShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_F3), this);
    ui->pushButtonBleDev->setIcon(AppGui::qtAwesome()->icon(fa::wrench));
    ui->pushButtonBleDev->setVisible(false);
    connect(m_BleDevTabShortcut, SIGNAL(activated()), this, SLOT(onBleDevTabShortcutActivated()));

    connect(wsClient, &WSClient::wsConnected, this, &MainWindow::updatePage);
    connect(wsClient, &WSClient::wsDisconnected, this, &MainWindow::updatePage);
    connect(wsClient, &WSClient::connectedChanged, this, &MainWindow::updatePage);
    connect(wsClient, &WSClient::statusChanged, this, &MainWindow::updatePage);

    connect(wsClient, &WSClient::memMgmtModeChanged, this, &MainWindow::enableCredentialsManagement);
    connect(ui->widgetCredentials, &CredentialsManagement::wantEnterMemMode, this, &MainWindow::wantEnterCredentialManagement);
    connect(ui->widgetCredentials, &CredentialsManagement::wantSaveMemMode, this, &MainWindow::wantSaveCredentialManagement);
    connect(this, &MainWindow::saveMMMChanges, ui->widgetCredentials, &CredentialsManagement::saveChanges);
    connect(ui->widgetFiles, &FilesManagement::wantEnterMemMode, this, &MainWindow::wantEnterCredentialManagement);
    connect(ui->widgetFiles, &FilesManagement::wantExitMemMode, this, &MainWindow::wantExitFilesManagement);

    connect(wsClient, &WSClient::statusChanged, [this]()
    {
        this->enableKnockSettings(wsClient->get_status() == Common::NoCardInserted);
        if (wsClient->get_status() == Common::UnkownSmartcad)
            ui->stackedWidget->setCurrentWidget(ui->pageSync);
    });

    ui->pushButtonExportFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonImportFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSettingsReset->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSettingsSave->setStyleSheet(CSS_BLUE_BUTTON);

    ui->pushButtonAutoStart->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonViewLogs->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonIntegrity->setStyleSheet(CSS_BLUE_BUTTON);
    ui->btnPassGenerationProfiles->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSubDomain->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonHIBP->setStyleSheet(CSS_BLUE_BUTTON);

    // Don't show the "check for updates" button when built from git directly.
    ui->pushButtonCheckUpdate->setVisible(QStringLiteral(APP_VERSION) != "git");

    ui->pushButtonCheckUpdate->setStyleSheet(CSS_BLUE_BUTTON);

    ui->pushButtonSettingsSave->setIcon(AppGui::qtAwesome()->icon(fa::floppyo, whiteButtons));
    ui->pushButtonSettingsReset->setIcon(AppGui::qtAwesome()->icon(fa::undo, whiteButtons));
    ui->pushButtonSettingsSave->setVisible(false);
    ui->pushButtonSettingsReset->setVisible(false);

    ui->pushButtonResetCard->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonImportCSV->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonExportCSV->setStyleSheet(CSS_BLUE_BUTTON);

    // temporary hide 'CSV Export' until it will be implemented
    ui->label_ExportCSV->hide();
    ui->label_ExportCSVHelp->hide();
    ui->pushButtonExportCSV->hide();

    connect(ui->pushButtonDevSettings, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonCred, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonSync, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonAbout, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonAppSettings, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonFiles, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonSSH, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonAdvanced, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonBleDev, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->btnPassGenerationProfiles, &QPushButton::clicked, [this]()
    {
        PassGenerationProfilesDialog dlg(this);
        dlg.setPasswordProfilesModel(m_passwordProfilesModel);
        dlg.exec();
    });

    ui->pushButtonDevSettings->setChecked(false);

    // DB Backups UI
    ui->toolButton_clearBackupFilePath->setIcon(AppGui::qtAwesome()->icon(fa::remove));
    ui->toolButton_setBackupFilePath->setIcon(AppGui::qtAwesome()->icon(fa::foldero));
    ui->lineEdit_dbBackupFilePath->setText(dbMasterController->getBackupFilePath());
    connect(dbMasterController, &DbMasterController::backupFilePathChanged, [=] (const QString &path)
    {
        ui->lineEdit_dbBackupFilePath->setText(path);
    });

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
    ui->comboBoxLang->addItem("en_DV", ID_KEYB_EN_DV_LUT);
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
    ui->comboBoxScreenBrightness->addItem("10%", 25);
    ui->comboBoxScreenBrightness->addItem("20%", 51);
    ui->comboBoxScreenBrightness->addItem("35%", 89);
    ui->comboBoxScreenBrightness->addItem("50%", 128);
    ui->comboBoxScreenBrightness->addItem("65%", 166);
    ui->comboBoxScreenBrightness->addItem("80%", 204);
    ui->comboBoxScreenBrightness->addItem("100%", 255);
    ui->comboBoxKnock->addItem(tr("Very Low"), 0);
    ui->comboBoxKnock->addItem(tr("Low"), 1);
    ui->comboBoxKnock->addItem(tr("Medium"), 2);
    ui->comboBoxKnock->addItem(tr("High"), 3);
    ui->comboBoxLoginOutput->addItem(tr("Tab"), 43);
    ui->comboBoxLoginOutput->addItem(tr("Enter"), 40);
    ui->comboBoxLoginOutput->addItem(tr("Space"), 44);
    ui->comboBoxPasswordOutput->addItem(tr("Tab"), 43);
    ui->comboBoxPasswordOutput->addItem(tr("Enter"), 40);
    ui->comboBoxPasswordOutput->addItem(tr("Space"), 44);

    // Close behavior
#ifdef Q_OS_MAC
    ui->closeBehaviorComboBox->addItem(
        tr("Close Application"), static_cast<int>(CloseBehavior::CloseApp));
    ui->closeBehaviorComboBox->addItem(
        tr("Hide Window"), static_cast<int>(CloseBehavior::HideWindow));
    ui->closeBehaviorComboBox->setCurrentIndex(
        s.value("settings/CloseBehavior",
                static_cast<int>(CloseBehavior::CloseApp)).toInt());

    connect(ui->closeBehaviorComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [this](int index)
    {
        Q_UNUSED(index);
        const auto behavior = ui->closeBehaviorComboBox->currentData().toInt();
        QSettings s;
        s.setValue("settings/CloseBehavior", behavior);
    });

    // On macOS Command+Q will terminate the program without calling the proper destructors if not
    // the following is defined.
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    auto *actionExit = fileMenu->addAction(tr("&Quit"));
    connect(actionExit, &QAction::triggered, this, [this]
    {
        const auto behavior =
            static_cast<CloseBehavior>(ui->closeBehaviorComboBox->currentData().toInt());
        switch (behavior)
        {
            case CloseBehavior::CloseApp:
                qApp->quit();
                break;

            case CloseBehavior::HideWindow:
                close();
                break;
        }
    });
#else
    ui->closeBehaviorLabel->setVisible(false);
    ui->closeBehaviorComboBox->setVisible(false);
#endif

    using LF = Common::LockUnlockModeFeatureFlags;

    ui->lockUnlockModeComboBox->addItem(tr("Disabled")          , (uint)LF::Disabled);
    ui->lockUnlockModeComboBox->addItem(tr("Password Only")     , (uint)LF::Password);
    ui->lockUnlockModeComboBox->addItem(tr("Login + Password")  , (uint)LF::Password|(uint)LF::Login);
    ui->lockUnlockModeComboBox->addItem(tr("Enter + Password")  , (uint)LF::Password|(uint)LF::SendEnter);
    ui->lockUnlockModeComboBox->addItem(tr("Password / Win + L")  , (uint)LF::Password|(uint)LF::SendWin_L);
    ui->lockUnlockModeComboBox->addItem(tr("Login + Pass / Win + L"), (uint)LF::Password|(uint)LF::Login|(uint)LF::SendWin_L);
    ui->lockUnlockModeComboBox->addItem(tr("Enter + Pass / Win + L"), (uint)LF::Password|(uint)LF::SendEnter|(uint)LF::SendWin_L);
    ui->lockUnlockModeComboBox->addItem(tr("Ctrl + Alt + Del / Win + L"), (uint)LF::SendCtrl_Alt_Del|(uint)LF::Password|(uint)LF::SendWin_L);
    ui->lockUnlockModeComboBox->setCurrentIndex(0);

    ui->comboBoxSystrayIcon->blockSignals(true);
    ui->comboBoxSystrayIcon->addItem(tr("Black"), tr(""));
    ui->comboBoxSystrayIcon->addItem(tr("White"), tr("_white"));
    int systrayIconIndex = ui->comboBoxSystrayIcon->findData(s.value("settings/systray_icon").toString());
    if (systrayIconIndex != -1) {
        ui->comboBoxSystrayIcon->setCurrentIndex(systrayIconIndex);
    }
    ui->comboBoxSystrayIcon->blockSignals(false);

    //When device has new parameters, update the GUI
    connect(wsClient, &WSClient::mpHwVersionChanged, [=]()
    {
        ui->widgetParamMini->setVisible(wsClient->isMPMini());
        ui->labelAbouHwSerial->setVisible(wsClient->isMPMini() || wsClient->isMPBLE());
        displayMCUVersion(wsClient->isMPBLE());
    });

    connect(wsClient, &WSClient::connectedChanged, this, &MainWindow::updateSerialInfos);
    connect(wsClient, &WSClient::fwVersionChanged, this, &MainWindow::updateSerialInfos);
    connect(wsClient, &WSClient::hwSerialChanged, this, &MainWindow::updateSerialInfos);
    connect(wsClient, &WSClient::hwMemoryChanged, this, &MainWindow::updateSerialInfos);

    connect(wsClient, &WSClient::memMgmtModeFailed, this, &MainWindow::memMgmtModeFailed);

    connect(wsClient, &WSClient::hwSerialChanged, [this](quint32 serial)
    {
        setUIDRequestInstructionsWithId(serial > 0 ? QString::number(serial) : "XXXX");
    });
    connect(wsClient, &WSClient::connectedChanged, [this] ()
    {
        setUIDRequestInstructionsWithId("XXXX");
    });

    QRegularExpressionValidator* uidKeyValidator = new QRegularExpressionValidator(QRegularExpression("[0-9A-Fa-f]{32}"), ui->UIDRequestKeyInput);
    ui->UIDRequestKeyInput->setValidator(uidKeyValidator);
    ui->UIDRequestValidateBtn->setEnabled(false);
    connect(ui->UIDRequestKeyInput, &QLineEdit::textEdited, [this] ()
    {
        ui->UIDRequestValidateBtn->setEnabled(ui->UIDRequestKeyInput->hasAcceptableInput());
    });

    gb_spinner = new QMovie(":/uid_spinner.gif",  {}, this);

    ui->UIDRequestResultLabel->setVisible(false);
    ui->UIDRequestResultIcon->setVisible(false);

    connect(ui->UIDRequestValidateBtn, &QPushButton::clicked, [this]()
    {
        if (wsClient)
        {
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

    connect(wsClient, &WSClient::uidChanged, [this](qint64 uid)
    {
        ui->UIDRequestResultLabel->setVisible(true);
        ui->UIDRequestResultIcon->setVisible(true);
        gb_spinner->stop();
        if (uid <= 0)
        {
            ui->UIDRequestResultIcon->setPixmap(QPixmap(":/message_error.png").scaledToHeight(ui->UIDRequestResultIcon->height(), Qt::SmoothTransformation));
            ui->UIDRequestResultLabel->setText("<span style='color:#FF0000; font-weight:bold'>" + tr("Either the device have been tempered with or the input key is invalid.") + "</span>");
            return;
        }
        ui->UIDRequestResultIcon->setPixmap(QPixmap(":/message_success.png").scaledToHeight(ui->UIDRequestResultIcon->height(), Qt::SmoothTransformation));
        ui->UIDRequestResultLabel->setText("<span style='color:#006400'>" + tr("Your device's UID is %1").arg(QString::number(uid, 16).toUpper()) + "</span>");
    });

    connect(wsClient, &WSClient::connectedChanged, [this](bool connected)
    {
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
        checkSettingsChanged();
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

    ui->checkBoxLockDevice->setChecked(s.value("settings/LockDeviceOnSystemEvents", true).toBool());
    connect(ui->checkBoxLockDevice, &QCheckBox::toggled, this, &MainWindow::onLockDeviceSystemEventsChanged);

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
    ui->widgetSpin->setPixmap(AppGui::qtAwesome()->icon(fa::circleonotch).pixmap(QSize(80, 80)));

    connect(&eventHandler, &SystemEventHandler::screenLocked, this, &MainWindow::onSystemEvents);
    connect(&eventHandler, &SystemEventHandler::loggingOff, this, &MainWindow::onSystemEvents);
    connect(&eventHandler, &SystemEventHandler::goingToSleep, this, &MainWindow::onSystemEvents);
    connect(&eventHandler, &SystemEventHandler::shuttingDown, this, &MainWindow::onSystemEvents);

    checkAutoStart();
    checkSubdomainSelection();
    checkHIBPSetting();

    //Check is ssh agent opt has to be checked
    ui->checkBoxSSHAgent->setChecked(s.value("settings/auto_start_ssh").toBool());
    ui->lineEditSshArgs->setText(s.value("settings/ssh_args").toString());

    ui->scrollArea->setStyleSheet("QScrollArea { background-color:transparent; }");
    ui->scrollAreaWidgetContents->setStyleSheet("#scrollAreaWidgetContents { background-color:transparent; }");

    // hide widget with prompts by default
    ui->promptWidget->setVisible(false);

    updateSerialInfos();
    updatePage();

    // Restore geometry and state from last session.
    restoreGeometry(s.value("MainWindow/geometry").toByteArray());
    restoreState(s.value("MainWindow/windowState").toByteArray());
}

MainWindow::~MainWindow()
{
    QSettings s;
    s.setValue("MainWindow/geometry", saveGeometry());
    s.setValue("MainWindow/windowState", saveState());

    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Leave MMM on main window closed
    if (wsClient != nullptr && wsClient->get_memMgmtMode())
    {
        if (!ui->widgetCredentials->isClean())
        {
            int ret = QMessageBox::warning(this, "Moolticute",
                                           tr("Credentials have been modified.\n"
                                              "Do you want to save your changes?"),
                                           QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (ret == QMessageBox::Yes)
            {
                emit saveMMMChanges();
            }
            else
            {
                wsClient->sendLeaveMMRequest();
            }
        }
    }

    event->accept();
    emit windowCloseRequested();
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::updateBackupControlsVisibility(bool visible)
{
    ui->label_backupControlsTitle->setVisible(visible);
    ui->label_dbBackupMonitoringHelp->setVisible(visible);
    ui->label_backupControlsDescription->setVisible(visible);
    ui->lineEdit_dbBackupFilePath->setVisible(visible);
    ui->toolButton_clearBackupFilePath->setVisible(visible);
    ui->toolButton_setBackupFilePath->setVisible(visible);
}

void MainWindow::updatePage()
{
    bool isCardUnknown = wsClient->get_status() == Common::UnkownSmartcad;

    ui->label_13->setVisible(!isCardUnknown);
    ui->label_14->setVisible(!isCardUnknown);
    ui->checkBoxExport->setVisible(!isCardUnknown);
    ui->pushButtonExportFile->setVisible(!isCardUnknown);
    ui->label_exportDBHelp->setVisible(!isCardUnknown);
    ui->label_MooltiAppHelp->setVisible(!isCardUnknown);

    ui->label_27->setVisible(!isCardUnknown);
    ui->label_29->setVisible(!isCardUnknown);
    ui->pushButtonIntegrity->setVisible(!isCardUnknown);
    ui->label_integrityCheckHelp->setVisible(!isCardUnknown);

    ui->groupBox_ResetCard->setVisible(isCardUnknown);

    // When an import db operation is peformed in an unknown card
    // don't change the page until the operation is finished
    if (ui->stackedWidget->currentWidget() == ui->pageWaiting &&
            ui->labelWait->text().contains("import_db_job"))
        ui->stackedWidget->setCurrentWidget(ui->pageWaiting);

    else if (ui->pushButtonAbout->isChecked())
        ui->stackedWidget->setCurrentWidget(ui->pageAbout);

    else if (ui->pushButtonAppSettings->isChecked())
        ui->stackedWidget->setCurrentWidget(ui->pageAppSettings);

    else if(!wsClient->isConnected())
        ui->stackedWidget->setCurrentWidget(ui->pageNoDaemon);

    else if (!wsClient->get_connected())
        ui->stackedWidget->setCurrentWidget(ui->pageNoConnect);

    else if (ui->pushButtonDevSettings->isChecked())
        ui->stackedWidget->setCurrentWidget(ui->pageSettings);

    else if (ui->pushButtonAdvanced->isChecked())
        ui->stackedWidget->setCurrentWidget(ui->pageAdvanced);

    else if (ui->pushButtonBleDev->isChecked())
        ui->stackedWidget->setCurrentWidget(ui->pageBleDev);

    else if (wsClient->get_status() == Common::NoCardInserted)
        ui->stackedWidget->setCurrentWidget(ui->pageMissingSecurityCard);

    else if (wsClient->get_status() == Common::Locked ||
            wsClient->get_status() == Common::LockedScreen)
        ui->stackedWidget->setCurrentWidget(ui->pageDeviceLocked);

    else if (ui->pushButtonCred->isChecked())
        ui->stackedWidget->setCurrentWidget(ui->pageCredentials);

    else if (ui->pushButtonSync->isChecked())
        ui->stackedWidget->setCurrentWidget(ui->pageSync);

    else if (ui->pushButtonFiles->isChecked())
        ui->stackedWidget->setCurrentWidget(ui->pageFiles);

    else if (ui->pushButtonSSH->isChecked())
        ui->stackedWidget->setCurrentWidget(ui->pageSSH);

    updateTabButtons();

    if (isCardUnknown)
        updateBackupControlsVisibility(false);
    else
        updateBackupControlsVisibility(dbBackupTrakingControlsVisible);

    if (wsClient->isMPBLE() && bBleDevTabVisible && !wsClient->isDeviceConnected())
    {
        emit onBleDevTabShortcutActivated();
    }
}

void MainWindow::enableKnockSettings(bool enable)
{
    ui->knockSettingsWidget->setEnabled(enable);

    ui->knockSettingsWidget->setToolTip(enable ? "" : tr("Remove the card from the device to change this setting."));
    ui->knockSettingsWidget->setToolTipDuration(enable ? -1 : std::numeric_limits<int>::max());

    ui->labelRemoveCard->setVisible(!ui->knockSettingsWidget->isEnabled());

    //Make sure the suffix label ("sensitivity") matches the color of the other widgets.
    const QString color =
            ui->checkBoxKnock->palette().color(enable ?
                                                   QPalette::Active : QPalette::Disabled, QPalette::ButtonText).name();

    ui->knockSettingsSuffixLabel->setStyleSheet(QStringLiteral("color: %1") .arg(color));
}

void MainWindow::updateSerialInfos() {
    const bool connected = wsClient->get_connected();

    ui->deviceInfoWidget->setVisible(connected);

    if(connected)
    {
        ui->labelAboutFwVersValue->setText(wsClient->get_fwVersion());
        ui->labelAbouHwSerialValue->setText(wsClient->get_hwSerial() > 0 ? QString::number(wsClient->get_hwSerial()) : NONE_STRING);
        ui->labelAbouHwMemoryValue->setText(wsClient->get_hwMemory() > 0 ? tr("%1Mb").arg(wsClient->get_hwMemory()): NONE_STRING);
        displayMCUVersion(wsClient->isMPBLE());
    }
    else
    {
        ui->labelAboutFwVersValue->setText(NONE_STRING);
        ui->labelAbouHwSerialValue->setText(NONE_STRING);
        ui->labelAbouHwMemoryValue->setText(NONE_STRING);
        wsClient->set_fwVersion(NONE_STRING);
        wsClient->set_hwSerial(0);
        wsClient->set_hwMemory(0);
        wsClient->set_auxMCUVersion(NONE_STRING);
        wsClient->set_mainMCUVersion(NONE_STRING);

    }
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

void MainWindow::onFilesAndSSHTabsShortcutActivated()
{
    bSSHKeyTabVisible = !bSSHKeyTabVisible;
    ui->pushButtonSSH->setVisible(bSSHKeyTabVisible);

    if (bSSHKeyTabVisible) {
        previousWidget = ui->stackedWidget->currentWidget();
    }
    else
    {
        if (previousWidget != nullptr)
            ui->stackedWidget->setCurrentWidget(previousWidget);
    }
}

void MainWindow::onAdvancedTabShortcutActivated()
{
    bAdvancedTabVisible = !bAdvancedTabVisible;
    ui->pushButtonAdvanced->setVisible(bAdvancedTabVisible);

    if (bAdvancedTabVisible) {
        previousWidget = ui->stackedWidget->currentWidget();
        ui->stackedWidget->setCurrentWidget(ui->pageAdvanced);
    }
    else
    {
        if ((previousWidget != nullptr) && (previousWidget != ui->pageFiles) && (previousWidget != ui->pageSSH))
            ui->stackedWidget->setCurrentWidget(previousWidget);
        else
            ui->stackedWidget->setCurrentWidget(ui->pageSettings);
    }
}

void MainWindow::onRadioButtonSSHTabsAlwaysToggled(bool bChecked)
{
    setKeysTabVisibleOnDemand(!bChecked);
}

void MainWindow::onBleDevTabShortcutActivated()
{
    if (!bBleDevTabVisible && (!wsClient->isMPBLE() || !wsClient->isDeviceConnected()))
    {
        qDebug() << "Ble Dev Tab is only available for BLE device.";
        return;
    }
    bBleDevTabVisible = !bBleDevTabVisible;
    ui->pushButtonBleDev->setVisible(bBleDevTabVisible);

    if (bBleDevTabVisible) {
        previousWidget = ui->stackedWidget->currentWidget();
        ui->stackedWidget->setCurrentWidget(ui->pageBleDev);
    }
    else
    {
        if (previousWidget != nullptr)
            ui->stackedWidget->setCurrentWidget(previousWidget);
        else
            ui->stackedWidget->setCurrentWidget(ui->pageSettings);

        ui->widgetBleDev->clearWidgets();
    }
}

void MainWindow::onCurrentTabChanged(int)
{
    QWidget *pCurrentWidget = ui->stackedWidget->currentWidget();
    if (pCurrentWidget != nullptr)
    {
        QPushButton *pButton = m_tabMap[pCurrentWidget];
        if (pButton != nullptr)
            pButton->setChecked(true);
    }
}

void MainWindow::wantEnterCredentialManagement()
{
    ui->labelWait->show();
    ui->labelWait->setText(tr("<html><!--enter_credentials_mgm_job--><head/><body><p><span style=\"font-size:12pt; font-weight:600;\">Waiting For Device Confirmation</span></p><p>Confirm the request on your device.</p></body></html>"));
    ui->stackedWidget->setCurrentWidget(ui->pageWaiting);
    ui->progressBarWait->hide();
    ui->labelProgressMessage->hide();

    connect(wsClient, &WSClient::progressChanged, this, &MainWindow::loadingProgress);

    updateTabButtons();
}

void MainWindow::wantSaveCredentialManagement()
{
    ui->labelWait->show();
    ui->labelWait->setText(tr("<html><!--save_credentials_job--><head/><body><p><span style=\"font-size:12pt; font-weight:600;\">Saving Changes to Device</span></p><p>Please wait.</p></body></html>"));
    ui->stackedWidget->setCurrentWidget(ui->pageWaiting);
    ui->progressBarWait->hide();
    ui->labelProgressMessage->hide();

    connect(wsClient, &WSClient::progressChanged, this, &MainWindow::loadingProgress);

    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(wsClient, &WSClient::credentialsUpdated, [this, conn](const QString & , const QString &, const QString &, bool success)
    {
        disconnect(*conn);
        if (!success)
        {
            QMessageBox::warning(this, tr("Failure"), tr("Couldn't save credentials, please contact the support team with moolticute's log"));
            ui->stackedWidget->setCurrentWidget(ui->pageCredentials);
        }

        disconnect(wsClient, &WSClient::progressChanged, this, &MainWindow::loadingProgress);
        updatePage();
        updateTabButtons();
    });
}

void MainWindow::wantImportDatabase()
{
    previousWidget = ui->stackedWidget->currentWidget();
    ui->labelWait->show();
    ui->labelWait->setText(tr("<html><!--import_db_job--><head/><body><p><span style=\"font-size:12pt; font-weight:600;\">Please Approve Request On Device</span></p></body></html>"));
    ui->stackedWidget->setCurrentWidget(ui->pageWaiting);
    ui->progressBarWait->hide();
    ui->labelProgressMessage->hide();

    connect(wsClient, &WSClient::progressChanged, this, &MainWindow::loadingProgress);
}

void MainWindow::wantExportDatabase()
{
    previousWidget = ui->stackedWidget->currentWidget();
    ui->widgetHeader->setEnabled(false);
    ui->labelWait->show();
    ui->labelWait->setText(tr("<html><!--export_db_job--><head/><body><p><span style=\"font-size:12pt; font-weight:600;\">Please Approve Request On Device</span></p></body></html>"));
    ui->stackedWidget->setCurrentWidget(ui->pageWaiting);
    ui->progressBarWait->hide();
    ui->labelProgressMessage->hide();

    connect(wsClient, &WSClient::progressChanged, this, &MainWindow::loadingProgress);
}

void MainWindow::handleBackupExported()
{
    ui->widgetHeader->setEnabled(true);
    disconnect(wsClient, &WSClient::progressChanged, this, &MainWindow::loadingProgress);

    ui->stackedWidget->setCurrentWidget(previousWidget);
}

void MainWindow::handleBackupImported()
{
    ui->widgetHeader->setEnabled(true);
    disconnect(wsClient, &WSClient::progressChanged, this, &MainWindow::loadingProgress);

    ui->stackedWidget->setCurrentWidget(previousWidget);
}

void MainWindow::showPrompt(PromptMessage * message)
{
    ui->promptWidget->setPromptMessage(message);
    ui->promptWidget->show();
}

void MainWindow::hidePrompt()
{
    ui->promptWidget->hide();
}

void MainWindow::showDbBackTrackingControls(const bool &show)
{
    dbBackupTrakingControlsVisible = show;
    updatePage();
}

void MainWindow::wantExitFilesManagement()
{
    ui->labelWait->show();
    ui->labelWait->setText(tr("<html><!--exit_file_mgm_job--><head/><body><p><span style=\"font-size:12pt; font-weight:600;\">Saving changes to device's memory</span></p><p>Please wait.</p></body></html>"));
    ui->stackedWidget->setCurrentWidget(ui->pageWaiting);
    ui->progressBarWait->hide();
    ui->labelProgressMessage->hide();

    connect(wsClient, &WSClient::progressChanged, this, &MainWindow::loadingProgress);
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(wsClient, &WSClient::deleteDataNodesFinished, [this, conn]()
                {
                    qDebug() << "Removing files is finished";
                    updatePage();
                    disconnect(wsClient, &WSClient::progressChanged, this, &MainWindow::loadingProgress);
                    disconnect(*conn);
                });

    updateTabButtons();
}

void MainWindow::loadingProgress(int total, int current, QString message)
{
    if (total != -1)
    {
        ui->progressBarWait->show();
        ui->progressBarWait->setMaximum(total);
        ui->progressBarWait->setValue(current);
    }
    else
    {
        ui->progressBarWait->setMinimum(0);
        ui->progressBarWait->setMaximum(0);
    }

    if (!message.isEmpty())
    {
        ui->labelProgressMessage->setVisible(true);
        ui->labelProgressMessage->setText(message);
    }
    else
        ui->labelProgressMessage->setVisible(false);

    if (ui->labelWait->text().contains("export_db_job"))
        ui->labelWait->setText(tr("<html><!--export_db_job--><head/><body><p><span style=\"font-size:12pt; font-weight:600;\">Exporting Database from Device</span></p><p>Please wait.</p></body></html>"));

    if (ui->labelWait->text().contains("import_db_job"))
        ui->labelWait->setText(tr("<html><!--import_db_job--><head/><body><p><span style=\"font-size:12pt; font-weight:600;\">Importing and Merging File to Device</span></p><p>Please wait.</p></body></html>"));

    if (ui->labelWait->text().contains("enter_credentials_mgm_job"))
        ui->labelWait->hide();
    else
        ui->labelWait->show();


}

void MainWindow::on_pushButtonAutoStart_clicked()
{
    QSettings s;

    bool en = s.value("settings/auto_start", true).toBool();

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

    //Autostart should be enabled by default
    bool en = s.value("settings/auto_start", true).toBool();

    AutoStartup::enableAutoStartup(en);
    ui->labelAutoStart->setText(tr("Start Moolticute with the computer: %1").arg((en?tr("Enabled"):tr("Disabled"))));
    if (en)
        ui->pushButtonAutoStart->setText(tr("Disable"));
    else
        ui->pushButtonAutoStart->setText(tr("Enable"));
}

void MainWindow::checkSubdomainSelection()
{
    QSettings s;

    bool en = s.value("settings/enable_subdomain_selection").toBool();

    ui->labelSubdomainSelection->setText(tr("Subdomain selection: %1").arg((en?tr("Enabled"):tr("Disabled"))));
    if (en)
        ui->pushButtonSubDomain->setText(tr("Disable"));
    else
        ui->pushButtonSubDomain->setText(tr("Enable"));
}

void MainWindow::checkHIBPSetting()
{
    QSettings s;

    bool en = s.value("settings/enable_hibp_check", false).toBool();

    ui->labelHIBPCheck->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
    ui->labelHIBPCheck->setOpenExternalLinks(true);
    ui->labelHIBPCheck->setText(tr("Integration with <a href=\"%1\">Have I Been Pwned</a>: %2")
                                .arg(HIBP_URL)
                                .arg((en?tr("Enabled"):tr("Disabled"))));
    if (en)
    {
        ui->pushButtonHIBP->setText(tr("Disable"));
    }
    else
    {
        ui->pushButtonHIBP->setText(tr("Enable"));
    }

}

void MainWindow::setKeysTabVisibleOnDemand(bool bValue)
{
    bSSHKeysTabVisibleOnDemand = bValue;

    // Save in settings
    QSettings s;
    s.setValue("settings/SSHKeysTabsVisibleOnDemand", bValue);

    // SSH keys tab will show up only after activating the CTRL+SHIFT+F1 shortcut
    if (bSSHKeysTabVisibleOnDemand) {
        bSSHKeyTabVisible = false;
        connect(m_FilesAndSSHKeysTabsShortcut, &QShortcut::activated, this, &MainWindow::onFilesAndSSHTabsShortcutActivated);
    }
    // SSH key tab always visible
    else {
        bSSHKeyTabVisible = true;
        disconnect(m_FilesAndSSHKeysTabsShortcut, &QShortcut::activated, this, &MainWindow::onFilesAndSSHTabsShortcutActivated);
    }

    ui->pushButtonSSH->setVisible(bSSHKeyTabVisible);
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
    if (ui->checkBoxExport->isChecked())
        wsClient->exportDbFile("none");
    else
        wsClient->exportDbFile("SimpleCrypt");

    // one-time connection, must be disconected immediately in the slot
    connect(wsClient, &WSClient::dbExported, this, &MainWindow::dbExported);

    wantExportDatabase();
}

void MainWindow::on_pushButtonImportFile_clicked()
{
    QSettings s;

    QString fname = QFileDialog::getOpenFileName(this, tr("Select database export..."),
                                                 s.value("last_used_path/import_dir", QDir::homePath()).toString(),
                                                 "Memory exports (*.bin);;All files (*.*)");
    if (fname.isEmpty())
        return;

    QFile f(fname);
    if (!f.open(QFile::ReadOnly))
    {
        QMessageBox::warning(this, tr("Error"), tr("Unable to read file %1").arg(fname));
        return;
    }

    s.setValue("last_used_path/import_dir", QFileInfo(fname).canonicalPath());

    ui->widgetHeader->setEnabled(false);
    wsClient->importDbFile(f.readAll(), ui->checkBoxImport->isChecked());
    connect(wsClient, &WSClient::dbImported, this, &MainWindow::dbImported);
    wantImportDatabase();
}

void MainWindow::dbExported(const QByteArray &d, bool success)
{
    // one-time connection
    disconnect(wsClient, &WSClient::dbExported, this, &MainWindow::dbExported);

    QSettings s;

    ui->widgetHeader->setEnabled(true);
    if (!success)
        QMessageBox::warning(this, tr("Error"), tr(d));
    else
    {
        QString fname = QFileDialog::getSaveFileName(this, tr("Save database export..."),
                                                     s.value("last_used_path/export_dir", QDir::homePath()).toString(),
                                                     "Memory exports (*.bin);;All files (*.*)");
        if (!fname.isEmpty())
        {
#if defined(Q_OS_LINUX)
            /**
             * getSaveFileName is using native dialog
             * On Linux it is not saving the choosen extension,
             * so need to add it from code.
             */
            const QString BIN_EXT = ".bin";
            if (!fname.endsWith(BIN_EXT))
            {
                fname += BIN_EXT;
            }
#endif
            QFile f(fname);
            if (!f.open(QFile::WriteOnly | QFile::Truncate))
                QMessageBox::warning(this, tr("Error"), tr("Unable to write to file %1").arg(fname));
            else
                f.write(d);
            f.close();

            s.setValue("last_used_path/export_dir", QFileInfo(fname).canonicalPath());
        }
    }
    ui->stackedWidget->setCurrentWidget(ui->pageSync);

    disconnect(wsClient, &WSClient::progressChanged, this, &MainWindow::loadingProgress);
}

void MainWindow::dbImported(bool success, QString message)
{
    ui->widgetHeader->setEnabled(true);
    disconnect(wsClient, &WSClient::dbImported, this, &MainWindow::dbImported);
    if (!success)
        QMessageBox::warning(this, tr("Error"), message);
    else
        QMessageBox::information(this, tr("Moolticute"), tr("Successfully imported and merged database into the device."));

    ui->stackedWidget->setCurrentWidget(ui->pageSync);

    disconnect(wsClient, &WSClient::progressChanged, this, &MainWindow::loadingProgress);
}

void MainWindow::on_pushButtonIntegrity_clicked()
{
    int r = QMessageBox::question(this, "Moolticute", tr("Do you want to start the integrity check of your device?"));
    if (r == QMessageBox::Yes)
    {
        wsClient->sendJsonData({{ "msg", "start_memcheck" }});

        connect(wsClient, SIGNAL(memcheckFinished(bool,int,int)),
                this, SLOT(integrityFinished(bool,int,int)));
        connect(wsClient, &WSClient::progressChanged, this, &MainWindow::integrityProgress);

        ui->widgetHeader->setEnabled(false);
        ui->labelWait->show();
        ui->labelWait->setText(tr("<html><!--check_integrity_job--><head/><body><p><span style=\"font-size:12pt; font-weight:600;\">Waiting For Device Confirmation</span></p><p>Confirm the request on your device.</p></body></html>"));
        ui->stackedWidget->setCurrentWidget(ui->pageWaiting);
        ui->progressBarWait->hide();
        ui->labelProgressMessage->hide();
        ui->integrityProgressMessageLabel->hide();
    }
}

void MainWindow::integrityProgress(int total, int current, QString message)
{
    if (ui->stackedWidget->currentWidget() == ui->pageWaiting)
    {
        ui->stackedWidget->setCurrentWidget(ui->pageIntegrity);
        ui->progressBarIntegrity->setMinimum(0);
        ui->progressBarIntegrity->setMaximum(0);
        ui->progressBarIntegrity->setValue(0);

        ui->integrityProgressMessageLabel->show();
    }

    ui->integrityProgressMessageLabel->setText(message);
    ui->progressBarIntegrity->setMaximum(total);
    ui->progressBarIntegrity->setValue(current);
}

void MainWindow::integrityFinished(bool success, int freeBlocks, int totalBlocks)
{
    disconnect(wsClient, SIGNAL(memcheckFinished(bool,int,int)),
               this, SLOT(integrityFinished(bool,int,int)));
    disconnect(wsClient, &WSClient::progressChanged, this, &MainWindow::integrityProgress);

    if (!success)
        QMessageBox::warning(this, "Moolticute", tr("Memory integrity check failed!"));
    else
    {
        const auto totalCreds = totalBlocks / 2;
        const auto usedCreds = (totalBlocks - freeBlocks) / 2;
        QMessageBox::information(this, "Moolticute",
            tr("Memory integrity check done successfully!\n%1 of %2 credential slots used.")
            .arg(usedCreds).arg(totalCreds));
    }
    ui->stackedWidget->setCurrentWidget(ui->pageSync);
    ui->widgetHeader->setEnabled(true);
}

void MainWindow::setUIDRequestInstructionsWithId(const QString & id)
{
    ui->UIDRequestLabel->setText(tr("To be sure that no one has tempered with your device, you can request a password which will allow you to fetch the UID of your device.<ol>"
                                    "<li>Get the serial number from the back of your device.</li>"
                                    "<li>&shy;<a href=\"mailto:support@themooltipass.com?subject=UID Request Code&body=My serial number is %1 and my order number is: FILL IN YOUR ORDER NO\">Send us an email</a> with the serial number and your order number, requesting the password.</li>"
                                    "<li>Enter the password you received from us</li></ol>").arg(id));
}

void MainWindow::enableCredentialsManagement(bool enable)
{
    if (enable && ui->stackedWidget->currentWidget() == ui->pageWaiting)
    {
        if (ui->pushButtonCred->isChecked())
            ui->stackedWidget->setCurrentWidget(ui->pageCredentials);
        else if (ui->pushButtonFiles->isChecked())
            ui->stackedWidget->setCurrentWidget(ui->pageFiles);
    }

    updateTabButtons();
}

void MainWindow::updateTabButtons()
{
    auto setEnabledToAllTabButtons = [=](bool enabled)
    {
        ui->widgetHeader->setEnabled(enabled);
        for (QObject * object: ui->widgetHeader->children())
        {
            if (typeid(*object) ==  typeid(QPushButton))
            {
                QAbstractButton *tabButton = qobject_cast<QAbstractButton *>(object);
                tabButton->setEnabled(enabled);
            }
        }
    };

    if (ui->stackedWidget->currentWidget() == ui->pageWaiting)
        setEnabledToAllTabButtons(false);

    // Enable or Disable tabs according to the device status
    if (wsClient->get_status() == Common::UnkownSmartcad)
    {
        // Enable all tab buttons
        setEnabledToAllTabButtons(true);

        ui->pushButtonCred->setEnabled(false);
        ui->pushButtonFiles->setEnabled(false);
        ui->pushButtonSSH->setEnabled(false);

        return;
    }

    if ((ui->stackedWidget->currentWidget() == ui->pageFiles
         || ui->stackedWidget->currentWidget() == ui->pageCredentials
         || ui->stackedWidget->currentWidget() == ui->pageIntegrity) &&
            wsClient->get_memMgmtMode())
    {
        // Disable all tab buttons
        setEnabledToAllTabButtons(false);

        ui->pushButtonCred->setEnabled(ui->stackedWidget->currentWidget() == ui->pageCredentials);
        ui->pushButtonFiles->setEnabled(ui->stackedWidget->currentWidget() == ui->pageFiles);

        return;
    }

    setEnabledToAllTabButtons(true);
}

void MainWindow::memMgmtModeFailed(int errCode, QString errMsg)
{
    Q_UNUSED(errCode)
    updatePage();
    showPrompt(new PromptMessage{"<b>" + tr("Memory Management Error") + "</b><br>" +
                                 tr("An error occured when trying to go into Memory Management mode.\n\n%1").arg(errMsg)});
}

void MainWindow::refreshAppLangCb()
{
    bool oldState = ui->comboBoxAppLang->blockSignals(true);

    ui->comboBoxAppLang->clear();

    QSettings settings;
    QString language = settings.value("settings/lang").toString();

    //Fill the language combobox from available *.qm files in the ressource
    ui->comboBoxAppLang->addItem(tr("System default language"), QStringLiteral(""));
    ui->comboBoxAppLang->addItem(QLocale::languageToString(QLocale::English), QStringLiteral("en"));
    if (language == "en") ui->comboBoxAppLang->setCurrentIndex(ui->comboBoxAppLang->count() - 1);

    /*
      Languages:
      the files contained into the .qrc ressources a automatically parsed to populate the UI.
      The filename of the translation files should have correct ISO 639 codes like:
          mc_CODE.qm
      for chinese, we need traditional chinese and simplified chinese. We need to use the following codes:
        mc_zh_HANS.qm for (generic) simplified Chinese characters
        mc_zh_HANT.qm for traditional Chinese characters
      other languages like french or german would only be:
        mc_fr.qm
        mc_de.qm
    */

    QDirIterator it(":/lang", QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        //havoc_LANG.qm
        QString fname = it.next();
        if (!fname.startsWith(":/lang/mc_")) continue;
        QString llang = fname.section('_', 1).section('.', 0, 0);
        QLocale locale(llang);

        QString ln;
        if (locale.language() == QLocale::Chinese)
            ln = QString("%1 (%2)")
                    .arg(QLocale::languageToString(locale.language()))
                    .arg(QLocale::scriptToString(locale.script()));
        else
            ln = locale.nativeLanguageName();

        ui->comboBoxAppLang->addItem(ln, llang);

        if (language == llang) ui->comboBoxAppLang->setCurrentIndex(ui->comboBoxAppLang->count() - 1);
    }

    ui->comboBoxAppLang->blockSignals(oldState);
}

void MainWindow::on_comboBoxAppLang_currentIndexChanged(int index)
{
    QSettings settings;
    QString language = settings.value("settings/lang").toString();
    QString lang = ui->comboBoxAppLang->itemData(index).toString();

    if (language != lang)
    {
        settings.setValue("settings/lang", lang);

        AppGui *a = qobject_cast<AppGui *>(qApp);
        a->setupLanguage();
        refreshAppLangCb();
    }
}

void MainWindow::on_pushButtonCheckUpdate_clicked()
{
    AppGui *a = qobject_cast<AppGui *>(qApp);
    a->checkUpdate(true);
}

void MainWindow::retranslateUi()
{
    ui->labelAboutVers->setText(ui->labelAboutVers->text().arg(APP_VERSION));
    updateSerialInfos();
}

void MainWindow::displayMCUVersion(bool visible)
{
    ui->labelAboutAuxMCU->setVisible(visible);
    ui->labelAboutAuxMCUValue->setVisible(visible);
    ui->labelAboutAuxMCUValue->setText(wsClient->get_auxMCUVersion());
    ui->labelAboutMainMCU->setVisible(visible);
    ui->labelAboutMainMCUValue->setVisible(visible);
    ui->labelAboutMainMCUValue->setText(wsClient->get_mainMCUVersion());
}

void MainWindow::on_toolButton_clearBackupFilePath_released()
{
    ui->lineEdit_dbBackupFilePath->clear();
    dbMasterController->setBackupFilePath("");
}

void MainWindow::on_toolButton_setBackupFilePath_released()
{
    QSettings s;
    QFileDialog dialog(this);
    dialog.setNameFilter(tr("Memory exports (*.bin)"));
    dialog.setAcceptMode(QFileDialog::AcceptOpen);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setDirectory(s.value("last_used_path/monitored_backup_dir", QDir::homePath()).toString());

    if (dialog.exec())
    {
        QString fname = dialog.selectedFiles().first();
        ui->lineEdit_dbBackupFilePath->setText(fname);
        dbMasterController->setBackupFilePath(fname);
        s.setValue("last_used_path/monitored_backup_dir", QFileInfo(fname).canonicalPath());
    }
}

void MainWindow::on_lineEditSshArgs_textChanged(const QString &)
{
    QSettings s;
    s.setValue("settings/ssh_args", ui->lineEditSshArgs->text());
}

void MainWindow::on_pushButtonResetCard_clicked()
{
    connect(wsClient, SIGNAL(cardResetFinished(bool)), this, SLOT(onResetCardFinished(bool)));
    wsClient->requestResetCard();
}

void MainWindow::onResetCardFinished(bool successfully)
{
    disconnect(wsClient, SIGNAL(resetCardFinished(bool)), this, SLOT(resetCardFinished(bool)));

    if (! successfully)
        QMessageBox::warning(this, "Moolticute", tr("Reset card failed!"));
    else
        QMessageBox::information(this, "Moolticute", tr("Reset card done successfully"));
}

void MainWindow::on_pushButtonImportCSV_clicked()
{
    QSettings s;

    QString fname = QFileDialog::getOpenFileName(this, tr("Select CSV file to import..."),
                                                 s.value("last_used_path/import_csv_dir", QDir::homePath()).toString(),
                                                 "CSV files (*.csv);;All files (*.*)");
    if (fname.isEmpty())
        return;

    QFile f(fname);
    if (! f.open(QFile::ReadOnly))
    {
        QMessageBox::warning(this, tr("Error"), tr("Unable to read file %1").arg(fname));
        return;
    }

    s.setValue("last_used_path/import_csv_dir", QFileInfo(fname).canonicalPath());

    QList<QStringList> readData;
    QString probe_separators = ",;.\t";
    QList<int> invalid_lines;

    foreach (QChar c, probe_separators)
    {
        qDebug() << "Probing read CSV with" << c << "as a separator";

        readData = QtCSV::Reader::readToList(fname, c);

        // empty file will be empty with any separator, stop immediately
        if (readData.size() == 0)
        {
            QMessageBox::warning(this, tr("Error"), tr("Nothing is read from %1").arg(fname));
            return;
        }

        invalid_lines.clear();

        // Check every line of CSV file contains only 3 rows
        for(int i = 0 ; i < readData.size() ; i++)
        {
            if (readData.at(i).size() != 3)
                invalid_lines.append(i+1);
        }

        if (invalid_lines.size() == 0)
        {
            qDebug() << "ImportCSV: CSV" << c << "delimiter detected";
            break;
        }

        // less than 10% of lines contains not 3 elements. It may be a password database
        if (invalid_lines.size() < (readData.size() * 0.5))
        {
            if (invalid_lines.size() < 10)
            {
                QStringList sl;
                foreach(int n, invalid_lines)
                    sl << QString("%1").arg(n);
                QMessageBox::warning(this, tr("Error"), tr("Unable to import %1: Each row must contain exact 3 items. Some lines don't (lines number: %2)").arg(fname).arg(sl.join(",")));
            }
            else
            {
                QMessageBox::warning(this, tr("Error"), tr("Unable to import %1: Each row must contain exact 3 items (more than 10 lines don't)").arg(fname));
            }
            return;
        }
    }

    if (invalid_lines.size() > 0)
    {
        QMessageBox::warning(this, tr("Error"), tr("Unable to import %1: Each row must contain exact 3 items using comma as a delimiter").arg(fname));
        return;
    }

    ui->widgetHeader->setEnabled(false);
    wsClient->importCSVFile(readData);
    connect(wsClient, &WSClient::dbImported, this, &MainWindow::dbImported);
    wantImportDatabase();
}

void MainWindow::onLockDeviceSystemEventsChanged(bool checked)
{
    QSettings s;
    s.setValue("settings/LockDeviceOnSystemEvents", checked);
}

void MainWindow::onSystemEvents()
{
    const bool exec = ui->checkBoxLockDevice->isChecked();
    if (exec && wsClient->get_status() == Common::Unlocked)
    {
        qDebug() << "System event. Locking device!";
        wsClient->sendLockDevice();
    }

    // In certain cases it is necessary to tell the system that it can proceed closing the
    // application down. It doesn't have any effect if the application wasn't about to be closed
    // down!
#ifdef Q_OS_MAC
    QTimer::singleShot(2 * 1000, &eventHandler, &SystemEventHandler::readyToTerminate);
#endif
}

void MainWindow::on_comboBoxSystrayIcon_currentIndexChanged(int index)
{
    QSettings s;
    s.setValue("settings/systray_icon", ui->comboBoxSystrayIcon->itemData(index).toString());
    emit iconChangeRequested();
}

void MainWindow::on_pushButtonSubDomain_clicked()
{
    QSettings s;

    bool en = s.value("settings/enable_subdomain_selection").toBool();

    int ret;
    if (en)
        ret = QMessageBox::question(this, "Moolticute", tr("Disable subdomain selection?"));
    else
        ret = QMessageBox::question(this, "Moolticute", tr("Enable subdomain selection?"));

    if (ret == QMessageBox::Yes)
    {
        s.setValue("settings/enable_subdomain_selection", !en);
        s.sync();

        checkSubdomainSelection();
    }
}

void MainWindow::on_pushButtonHIBP_clicked()
{
    QSettings s;

    bool en = s.value("settings/enable_hibp_check", false).toBool();

    int ret;
    if (en)
    {
        ret = QMessageBox::question(this, "Moolticute", tr("Disable Have I Been Pwned check?"));
    }
    else
    {
        ret = QMessageBox::question(this, "Moolticute", tr("Enable Have I Been Pwned check?"));
    }

    if (ret == QMessageBox::Yes)
    {
        s.setValue("settings/enable_hibp_check", !en);
        s.sync();

        checkHIBPSetting();
    }
}
