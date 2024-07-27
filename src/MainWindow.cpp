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
#include "SettingsGuiHelper.h"
#include "DeviceDetector.h"
#include "SystemNotifications/SystemNotification.h"

#include "qtcsv/reader.h"

const QString MainWindow::NONE_STRING = tr("None");
const QString MainWindow::TAB_STRING = tr("Tab");
const QString MainWindow::ENTER_STRING = tr("Enter");
const QString MainWindow::SPACE_STRING = tr("Space");
const QString MainWindow::DEFAULT_KEY_STRING = tr("Default Key");
const QString MainWindow::MANUAL_STRING = "<a href=\"%1\">" + tr("User Manual") + "</a>";
const QString MainWindow::BLE_MANUAL_URL = "https://raw.githubusercontent.com/mooltipass/minible/master/MooltipassMiniBLEUserManual.pdf";
const QString MainWindow::MINI_MANUAL_URL = "https://raw.githubusercontent.com/limpkin/mooltipass/master/user_manual_mini.pdf";
const QString MainWindow::BUNDLE_OUTDATED_TEXT = tr("Bundle v%1 is available <a href=\"https://www.themooltipass.com/updates/index.php?sn=%2&bundlev=%3\">here</a>");
const QString MainWindow::SERIAL_STR_START = "MOOLTIP";

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
    m_tabMap[ui->pageFido] = ui->pushButtonFido;
    m_tabMap[ui->pageNotes] = ui->pushButtonNotes;
    connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);

    ui->widgetCredentials->setWsClient(wsClient);
    ui->widgetFiles->setWsClient(wsClient);
    ui->widgetSSH->setWsClient(wsClient);
    ui->widgetBleDev->setWsClient(wsClient);
    ui->widgetFido->setWsClient(wsClient);
    ui->widgetNotes->setWsClient(wsClient);

    ui->widgetCredentials->setPasswordProfilesModel(m_passwordProfilesModel);

    ui->labelAboutVers->setText(ui->labelAboutVers->text().arg(APP_VERSION));

    ui->labelAboutAuxMCU->setText(tr("Aux MCU version:"));
    ui->labelAboutMainMCU->setText(tr("Main MCU version:"));

    ui->widgetNotFlashedWarning->hide();
    ui->labelNotFlashedWarningIcon->setPixmap(AppGui::qtAwesome()->icon(fa::warning).pixmap(QSize(20, 20)));
    ui->labelSerialNumberIncorrect->setStyleSheet("QLabel {color: blue;}");
    connect(ui->labelSerialNumberIncorrect, &ClickableLabel::clicked, this, &MainWindow::onIncorrectSerialNumberClicked);

    ui->pbBleBattery->setStyleSheet("border: 1px solid black");
    ui->pbBleBattery->hide();

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
    connect(ui->labelLogo, &ClickableLabel::rightClicked, this, &MainWindow::onDisplayHiddenTabs);
    ui->pushButtonAdvanced->setVisible(bAdvancedTabVisible);

    ui->pushButtonFido->setIcon(AppGui::qtAwesome()->icon(fa::usb));
    ui->pushButtonFido->setVisible(false);

    ui->pushButtonNotes->setIcon(AppGui::qtAwesome()->icon(fa::newspapero));
    ui->pushButtonNotes->setVisible(false);

    unsigned int keyModifiers = Qt::SHIFT;

    m_FilesAndSSHKeysTabsShortcut = new QShortcut(QKeySequence(keyModifiers | Qt::Key_F1), this);
    setKeysTabVisibleOnDemand(bSSHKeysTabVisibleOnDemand);
    connect(ui->radioButtonSSHTabAlways, &QRadioButton::toggled, this, &MainWindow::onRadioButtonSSHTabsAlwaysToggled);
    m_advancedTabShortcut = new QShortcut(QKeySequence(keyModifiers | Qt::Key_F2), this);
    connect(m_advancedTabShortcut, SIGNAL(activated()), this, SLOT(onAdvancedTabShortcutActivated()));
    m_BleDevTabShortcut = new QShortcut(QKeySequence(keyModifiers | Qt::Key_F3), this);
    ui->pushButtonBleDev->setIcon(AppGui::qtAwesome()->icon(fa::wrench));
    ui->pushButtonBleDev->setVisible(false);
    connect(m_BleDevTabShortcut, SIGNAL(activated()), this, SLOT(onBleDevTabShortcutActivated()));

    connect(wsClient, &WSClient::wsConnected, this, &MainWindow::updatePage);
    connect(wsClient, &WSClient::wsDisconnected, this, &MainWindow::updatePage);
    connect(wsClient, &WSClient::connectedChanged, this, &MainWindow::updatePage);
    connect(wsClient, &WSClient::statusChanged, this, &MainWindow::updatePage);
    connect(wsClient, &WSClient::deviceConnected, this, &MainWindow::onDeviceConnected);
    connect(wsClient, &WSClient::deviceDisconnected, this, &MainWindow::onDeviceDisconnected);

    connect(wsClient, &WSClient::memMgmtModeChanged, this, &MainWindow::enableCredentialsManagement);
    connect(ui->widgetCredentials, &CredentialsManagement::wantEnterMemMode, this, &MainWindow::wantEnterCredentialManagement);
    connect(ui->widgetCredentials, &CredentialsManagement::wantSaveMemMode, this, &MainWindow::wantSaveCredentialManagement);
    connect(this, &MainWindow::saveMMMChanges, ui->widgetCredentials, &CredentialsManagement::saveChanges);
    connect(ui->widgetFiles, &FilesManagement::wantEnterMemMode, this, &MainWindow::wantEnterCredentialManagement);
    connect(ui->widgetFiles, &FilesManagement::wantExitMemMode, this, &MainWindow::wantExitFilesManagement);
    connect(ui->widgetFido, &FidoManagement::wantEnterMemMode, this, &MainWindow::wantEnterCredentialManagement);
    connect(ui->widgetFido, &FidoManagement::wantExitMemMode, this, &MainWindow::wantExitFidoManagement);
    connect(ui->widgetNotes, &NotesManagement::changeNote, this, &MainWindow::wantChangeNote);

    connect(wsClient, &WSClient::noteSaved, this, &MainWindow::displayNotePage);
    connect(wsClient, &WSClient::noteReceived, this, &MainWindow::displayNotePage);
    connect(wsClient, &WSClient::noteDeleted, this, &MainWindow::displayNotePage);
    connect(ui->widgetNotes, &NotesManagement::updateTabs, [this](){ updateTabButtons(); });

    connect(wsClient, &WSClient::memMgmtModeChanged, [this](bool isMMM){
        if (isMMM && (ui->promptWidget->isMMMErrorPrompt()
                  || ui->promptWidget->isBackupPrompt()))
        {
            ui->promptWidget->hide();
        }
    });

    connect(wsClient, &WSClient::statusChanged, [this](Common::MPStatus status)
    {
        if (status == Common::UnknownSmartcard)
            ui->stackedWidget->setCurrentWidget(ui->pageSync);

        if (wsClient->isMPBLE())
        {
            if (Common::Unlocked == status)
            {
                ui->settings_user_language->show();
                ui->settings_bt_layout->show();
                ui->settings_usb_layout->show();
                wsClient->sendLoadParams();
                if (!m_notesFetched && wsClient->get_bundleVersion() > Common::BLE_BUNDLE_WITH_NOTES)
                {
                    m_notesFetched = true;
                    wsClient->sendFetchNotes();
                }
                auto currentCategoryIndex = ui->comboBoxBleCurrentCategory->currentIndex();
                if (currentCategoryIndex != 0)
                {
                    // When enforce current category is set sending category number to device
                    wsClient->sendCurrentCategory(currentCategoryIndex);
                }
            }
            else
            {
                ui->settings_user_language->hide();
                ui->settings_bt_layout->hide();
                ui->settings_usb_layout->hide();
            }
        }
        else
        {
            enableKnockSettings(status == Common::NoCardInserted);
            if (Common::Unlocked == status)
            {
                wsClient->settingsHelper()->resetSettings();
            }
        }
    });

#ifdef Q_OS_MAC
    // For Mac clipboard changed event is not working when app is in background,
    // so need to check the clipboard content when window is activated.
    connect(this, &MainWindow::mainWindowActivated,
            ui->widgetCredentials, &CredentialsManagement::onMainWindowActivated);
#endif

    ui->pushButtonExportFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonImportFile->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSettingsReset->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSettingsSave->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSettingsSetToDefault->setStyleSheet(CSS_BLUE_BUTTON);

    ui->pushButtonAutoStart->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonViewLogs->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonIntegrity->setStyleSheet(CSS_BLUE_BUTTON);
    ui->btnPassGenerationProfiles->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonSubDomain->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonHIBP->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonNiMHRecondition->setStyleSheet(CSS_BLUE_BUTTON);

    // Don't show the "check for updates" button when built from git directly.
    // Also deb packages are handled by our PPA, disable check
    ui->pushButtonCheckUpdate->setVisible(QStringLiteral(APP_VERSION) != "git" &&
                                          QStringLiteral(APP_TYPE) != "deb");

    ui->pushButtonCheckUpdate->setStyleSheet(CSS_BLUE_BUTTON);

    ui->pushButtonSettingsSave->setIcon(AppGui::qtAwesome()->icon(fa::floppyo, whiteButtons));
    ui->pushButtonSettingsReset->setIcon(AppGui::qtAwesome()->icon(fa::undo, whiteButtons));
    ui->pushButtonSettingsSetToDefault->setIcon(AppGui::qtAwesome()->icon(fa::repeat, whiteButtons));
    ui->pushButtonSettingsSave->setVisible(false);
    ui->pushButtonSettingsReset->setVisible(false);
    ui->pushButtonSettingsSetToDefault->setVisible(false);

    ui->pushButtonResetCard->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonImportCSV->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonExportCSV->setStyleSheet(CSS_BLUE_BUTTON);
    ui->pushButtonGetAvailableUsers->setStyleSheet(CSS_BLUE_BUTTON);
    connect(wsClient, &WSClient::displayAvailableUsers,
            [this](const QString& num)
            {
                ui->lineEdit_AvailableUsers->setText(num);
            });
    connect(wsClient, &WSClient::connectedChanged, this,
            [this]()
            {
                ui->lineEdit_AvailableUsers->setText("");
                m_firstBatteryPctReceived = BatteryNotiStatus::NO_STATUS;
            });
    connect(wsClient, &WSClient::updateBatteryPercent,
            [this](int battery)
            {
                DeviceDetector::instance().setBattery(battery);
                ui->pbBleBattery->setValue(battery);
                if (battery < BATTERY_WARNING_LIMIT && !ui->label_charging->isVisible() &&
                        BatteryNotiStatus::VALID_BATTERY_RECIEVED == m_firstBatteryPctReceived)
                {
                    SystemNotification::instance().createNotification(tr("Low Battery"), tr("Battery is below %1%, please charge your Mooltipass.").arg(BATTERY_WARNING_LIMIT));
                    m_firstBatteryPctReceived = BatteryNotiStatus::LOW_BATTERY_DISPLAYED;
                }
                // Only display low battery warning after we received a valid battery pct
                if (BatteryNotiStatus::NO_STATUS == m_firstBatteryPctReceived)
                {
                    m_firstBatteryPctReceived = BatteryNotiStatus::VALID_BATTERY_RECIEVED;
                }
            });

    connect(wsClient, &WSClient::updateChargingStatus,
            [this](bool charging)
            {
                if (charging)
                {
                    ui->label_charging->show();
                }
                else
                {
                    ui->label_charging->hide();
                    m_firstBatteryPctReceived = BatteryNotiStatus::NO_STATUS;
                }
            });

    connect(wsClient, &WSClient::reconditionFinished, this, &MainWindow::onReconditionFinished);

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
    connect(ui->pushButtonFido, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
    connect(ui->pushButtonNotes, SIGNAL(clicked(bool)), this, SLOT(updatePage()));
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

    fillBLEBrightnessComboBox(ui->comboBoxUsbScreenBrightness);
    fillBLEBrightnessComboBox(ui->comboBoxBatteryScreenBrightness);

    ui->comboBoxInactivityTimer->addItem(tr("No Inactivity"), 0);
    ui->comboBoxInactivityTimer->addItem(tr("5 minutes"), 5);
    ui->comboBoxInactivityTimer->addItem(tr("10 minutes"), 10);
    ui->comboBoxInactivityTimer->addItem(tr("15 minutes"), 15);
    ui->comboBoxInactivityTimer->addItem(tr("30 minutes"), 30);

    ui->comboBoxInformationTimeDelay->addItem(tr("0.5 second"), 5);
    ui->comboBoxInformationTimeDelay->addItem(tr("1 second"), 10);
    ui->comboBoxInformationTimeDelay->addItem(tr("2 seconds"), 20);
    ui->comboBoxInformationTimeDelay->addItem(tr("3 seconds"), 30);
    ui->comboBoxInformationTimeDelay->addItem(tr("4 seconds"), 40);
    ui->comboBoxInformationTimeDelay->addItem(tr("5 seconds"), 50);

    ui->comboBoxScreensaverId->addItem(tr("None"), 0);
    ui->comboBoxScreensaverId->addItem(tr("Nyancat"), 1);

    ui->comboBoxMCSubdomainForceStatus->addItem(tr("Let moolticute decide"), 0);
    ui->comboBoxMCSubdomainForceStatus->addItem(tr("Force subdomain support"), 1);
    ui->comboBoxMCSubdomainForceStatus->addItem(tr("Force subdomain ignore"), 2);

    ui->comboBoxDelayBefUnlockLogin->addItem("100", 10);
    ui->comboBoxDelayBefUnlockLogin->addItem("250", 25);
    ui->comboBoxDelayBefUnlockLogin->addItem("600", 60);
    ui->comboBoxDelayBefUnlockLogin->addItem("1000", 100);
    ui->comboBoxDelayBefUnlockLogin->addItem("1500", 150);
    ui->comboBoxDelayBefUnlockLogin->addItem("2000", 200);

    // Close behavior
#ifdef Q_OS_MAC

    // On macOS Command+Q will terminate the program without calling the proper destructors if not
    // the following is defined.
    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    auto *actionExit = fileMenu->addAction(tr("&Quit"));
    connect(actionExit, &QAction::triggered, this, [this]
               { close(); });
#endif

    fillLockUnlockItems();

    ui->comboBoxSystrayIcon->blockSignals(true);
    ui->comboBoxSystrayIcon->addItem(tr("Black"), tr(""));
    ui->comboBoxSystrayIcon->addItem(tr("White"), tr("_white"));
    int systrayIconIndex = ui->comboBoxSystrayIcon->findData(s.value("settings/systray_icon").toString());
    if (systrayIconIndex != -1) {
        ui->comboBoxSystrayIcon->setCurrentIndex(systrayIconIndex);
    }
    ui->comboBoxSystrayIcon->blockSignals(false);

    fillInitialCurrentCategories();
    connect(wsClient, &WSClient::displayUserCategories, this, &MainWindow::setCurrentCategoryOptions);
    connect(wsClient, &WSClient::updateCurrentCategories, this, &MainWindow::setCurrentCategoryOptions);

    ui->cbLoginPrompt->setDisabled(true);
    ui->cbPinForMMM->setDisabled(true);
    ui->cbStoragePrompt->setDisabled(true);
    ui->cbAdvancedMenu->setDisabled(true);
    ui->cbBluetoothEnabled->setDisabled(true);
    ui->cbKnockDisabled->setDisabled(true);

    connect(wsClient, &WSClient::advancedMenuChanged,
            &DeviceDetector::instance(), &DeviceDetector::onAdvancedModeChanged);
    connect(wsClient, &WSClient::advancedMenuChanged, this, &MainWindow::handleAdvancedModeChange);
    handleAdvancedModeChange(wsClient->get_advancedMenu());

    connect(wsClient, &WSClient::updateUserSettingsOnUI,
            [this](const QJsonObject& settings)
            {
                ui->cbLoginPrompt->setChecked(settings["login_prompt"].toBool());
                ui->cbPinForMMM->setChecked(settings["pin_for_mmm"].toBool());
                ui->cbStoragePrompt->setChecked(settings["storage_prompt"].toBool());
                bool advancedMenu = settings["advanced_menu"].toBool();
                ui->cbAdvancedMenu->setChecked(advancedMenu);
                wsClient->set_advancedMenu(advancedMenu);
                ui->cbBluetoothEnabled->setChecked(settings["bluetooth_enabled"].toBool());
                ui->cbKnockDisabled->setChecked(!settings["knock_disabled"].toBool());
            });

    connect(wsClient, &WSClient::updateBLEDeviceLanguage,
            [this](const QJsonObject& langs)
            {
                if (shouldUpdateItems(m_languagesCache, langs))
                {
                    updateBLEComboboxItems(ui->comboBoxDeviceLang, langs);
                    updateBLEComboboxItems(ui->comboBoxUserLanguage, langs);
                }
            }
    );
    connect(wsClient, &WSClient::updateBLEKeyboardLayout,
            [this](const QJsonObject& layouts)
            {
                if (shouldUpdateItems(m_keyboardLayoutCache, layouts))
                {
                    updateBLEComboboxItems(ui->comboBoxUsbLayout, layouts);
                    updateBLEComboboxItems(ui->comboBoxBtLayout, layouts);
                }
                wsClient->settingsHelper()->resetSettings();
            }
    );


    //When device has new parameters, update the GUI
    connect(wsClient, &WSClient::mpHwVersionChanged, [=]()
    {
        ui->widgetParamMini->setVisible(wsClient->isMPMini());
        ui->labelAbouHwSerial->setVisible(wsClient->isMPMini() || wsClient->isMPBLE());
        displayMCUVersion(wsClient->isMPBLE());
        if (!wsClient->isFw12() && !wsClient->isMPBLE())
        {
            ui->groupBox_Information->hide();
        }
        updateDeviceDependentUI();
    });

    connect(wsClient, &WSClient::fwVersionChanged, wsClient->settingsHelper(), &SettingsGuiHelper::checkKeyboardLayout);
    connect(wsClient, &WSClient::connectedChanged, this, &MainWindow::updateSerialInfos);
    connect(wsClient, &WSClient::fwVersionChanged, this, &MainWindow::updateSerialInfos);
    connect(wsClient, &WSClient::hwSerialChanged, this, &MainWindow::updateSerialInfos);
    connect(wsClient, &WSClient::hwMemoryChanged, this, &MainWindow::updateSerialInfos);
    connect(wsClient, &WSClient::platformSerialChanged, this, &MainWindow::updateSerialInfos);
    connect(wsClient, &WSClient::bundleVersionChanged, this, &MainWindow::displayBundleVersion);
    connect(wsClient, &WSClient::bundleVersionChanged, this, &MainWindow::sendRequestNotes);
    connect(wsClient, &WSClient::bundleVersionChanged, this, &MainWindow::onBundleVersionChanged);

    connect(wsClient, &WSClient::memMgmtModeFailed, this, &MainWindow::memMgmtModeFailed);

    connect(wsClient, &WSClient::hwSerialChanged, [this](quint32 serial)
    {
        QString serialStr = serial > 0 ? QString::number(serial) : "XXXX";
        if (wsClient->isMPBLE())
        {
            QString bundleStr = QString::number(wsClient->get_bundleVersion());
            setSecurityChallengeText(serialStr, bundleStr);
            ui->labelBundleOutdatedText->setText(BUNDLE_OUTDATED_TEXT.arg(Common::BLE_LATEST_BUNDLE_VERSION).arg(serial).arg(bundleStr));
        }
        else
        {
            setUIDRequestInstructionsWithId(serialStr);
        }
    });
    connect(wsClient, &WSClient::bundleVersionChanged, [this](int bundleVersion)
    {
        auto serial = wsClient->get_hwSerial();
        ui->labelBundleOutdatedText->setText(BUNDLE_OUTDATED_TEXT.arg(Common::BLE_LATEST_BUNDLE_VERSION).arg(serial).arg(bundleVersion));
        if (wsClient->isMPBLE())
        {
            QString serialStr = serial > 0 ? QString::number(serial) : "XXXX";
            setSecurityChallengeText(serialStr, QString::number(bundleVersion));
        }
    });


    connect(wsClient, &WSClient::connectedChanged, [this] ()
    {
        if (wsClient->isMPBLE())
        {
            setSecurityChallengeText("XXXX", "XXXX");
        }
        else
        {
            setUIDRequestInstructionsWithId("XXXX");
        }
    });

    connect(wsClient, &WSClient::displayMiniImportWarning, this, &MainWindow::displayMiniImportWarning);

    QRegularExpressionValidator* uidKeyValidator = new QRegularExpressionValidator(QRegularExpression(Common::HEX_REGEXP.arg(UID_REQUEST_LENGTH)), ui->UIDRequestKeyInput);
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
        bool isBle = wsClient->isMPBLE();
        ui->UIDRequestGB->setVisible(connected && !isBle);
        gb_spinner->stop();
        ui->UIDRequestResultLabel->setMovie(nullptr);
        ui->UIDRequestResultLabel->setText({});
        ui->UIDRequestResultLabel->setVisible(false);
        ui->UIDRequestResultIcon->setVisible(false);
        ui->groupBoxSecurityChallenge->setVisible(connected && isBle);
        ui->labelSecurityChallengeResult->setText("");
        ui->labelSecurityChallengeResult->setVisible(false);
        ui->labelSecurityChallengeIcon->setVisible(false);
    });

    ui->UIDRequestGB->hide();
    ui->groupBoxSecurityChallenge->hide();

    ui->lineEditChallengeString->setValidator(new QRegularExpressionValidator(
                                                  QRegularExpression(Common::HEX_REGEXP.arg(SECURITY_CHALLENGE_LENGTH)),
                                                  ui->UIDRequestKeyInput));

    connect(ui->lineEditChallengeString, &QLineEdit::textEdited, [this] ()
    {
        ui->pushButtonSecurityValidate->setEnabled(ui->lineEditChallengeString->hasAcceptableInput());
    });

    connect(wsClient, &WSClient::challengeResultReceived, [this](QString res)
    {
        ui->labelSecurityChallengeIcon->setVisible(false);
        gb_spinner->stop();
        ui->labelSecurityChallengeResult->setText(res);
    });
    connect(wsClient, &WSClient::challengeResultFailed, [this]()
    {
        ui->labelSecurityChallengeIcon->setVisible(false);
        gb_spinner->stop();
        ui->labelSecurityChallengeResult->setText("");
        QMessageBox::warning(this, tr("Security Challenge"),
                             tr("Security Challenge Failed"));
    });

    ui->checkBoxLockOnSleep->setChecked(s.value("settings/LockDeviceOnSystemEvents", true).toBool());
    connect(ui->checkBoxLockOnSleep, &QCheckBox::toggled, this, &MainWindow::onLockDeviceSystemEventsChanged);

    m_keyboardBTLayoutOrigValue = s.value(Common::SETTING_BT_LAYOUT_ENFORCE, false).toBool();
    m_keyboardBTLayoutActualValue = m_keyboardBTLayoutOrigValue;
    ui->checkBoxEnforceBTLayout->setChecked(m_keyboardBTLayoutOrigValue);
    m_keyboardUsbLayoutOrigValue = s.value(Common::SETTING_USB_LAYOUT_ENFORCE, false).toBool();
    m_keyboardUsbLayoutActualValue = m_keyboardUsbLayoutOrigValue;
    ui->checkBoxEnforceUSBLayout->setChecked(m_keyboardUsbLayoutOrigValue);

    connect(wsClient, &WSClient::bleNameChanged, this, &MainWindow::onBleNameChanged);

    connect(wsClient, &WSClient::serialNumberChanged, this, &MainWindow::onSerialNumberChanged);

    wsClient->settingsHelper()->setMainWindow(this);
#ifdef Q_OS_WIN
    const auto keyboardLayoutWidth = 150;
    ui->comboBoxBtLayout->setMinimumWidth(keyboardLayoutWidth);
    ui->comboBoxUsbLayout->setMinimumWidth(keyboardLayoutWidth);
#endif

    //Setup the confirm view
    ui->widgetSpin->setPixmap(AppGui::qtAwesome()->icon(fa::circleonotch).pixmap(QSize(80, 80)));

    connect(&eventHandler, &SystemEventHandler::screenLocked, this, &MainWindow::onSystemEvents);
    connect(&eventHandler, &SystemEventHandler::loggingOff, this, &MainWindow::onSystemEvents);
    connect(&eventHandler, &SystemEventHandler::goingToSleep, this, &MainWindow::onSystemEvents);
    connect(&eventHandler, &SystemEventHandler::shuttingDown, this, &MainWindow::onSystemEvents);
    connect(&eventHandler, &SystemEventHandler::screenUnlocked, this, &MainWindow::onSystemUnlock);

    checkAutoStart();
    checkSubdomainSelection();
    checkHIBPSetting();

    //Check is ssh agent opt has to be checked
    ui->checkBoxSSHAgent->setChecked(s.value("settings/auto_start_ssh").toBool());
    ui->checkBoxTLDCheck->setChecked(s.value("settings/disable_tld_check").toBool());
    ui->lineEditSshArgs->setText(s.value("settings/ssh_args").toString());

    ui->scrollArea->setStyleSheet("QScrollArea { background-color:transparent; }");
    ui->scrollAreaMCSettings->setStyleSheet("QScrollArea { background-color:transparent; }");
    ui->scrollAreaWidgetContents->setStyleSheet("#scrollAreaWidgetContents { background-color:transparent; }");
    ui->scrollAreaMCSettingsContents->setStyleSheet("#scrollAreaMCSettingsContents { background-color:transparent; }");

    // hide widget with prompts by default
    ui->promptWidget->setVisible(false);

    ui->label_charging->setPixmap(QString::fromUtf8(":/charge.png"));
    ui->label_charging->setMaximumSize(13,25);
#if QT_VERSION < 0x060000
    ui->label_charging->setPixmap(ui->label_charging->pixmap()->scaled(13,25, Qt::KeepAspectRatio));
#else
    ui->label_charging->setPixmap(ui->label_charging->pixmap().scaled(13,25, Qt::KeepAspectRatio));
#endif
    ui->label_charging->hide();

    ui->label_UserManual->hide();
    ui->label_UserManual->setTextFormat(Qt::RichText);
    ui->label_UserManual->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->label_UserManual->setOpenExternalLinks(true);

    ui->labelBundleOutdatedWarning->hide();
    ui->labelBundleOutdatedWarning->setPixmap(AppGui::qtAwesome()->icon(fa::warning).pixmap(QSize(20, 20)));
    ui->labelBundleOutdatedWarning2->hide();
    ui->labelBundleOutdatedWarning2->setPixmap(AppGui::qtAwesome()->icon(fa::warning).pixmap(QSize(20, 20)));
    ui->labelBundleOutdatedText->hide();
    ui->labelBundleOutdatedText->setTextFormat(Qt::RichText);
    ui->labelBundleOutdatedText->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->labelBundleOutdatedText->setOpenExternalLinks(true);

    updateSerialInfos();
    updatePage();

    // Restore geometry and state from last session.
    restoreGeometry(s.value("MainWindow/geometry").toByteArray());
    restoreState(s.value("MainWindow/windowState").toByteArray());

    ui->checkBoxDebugHttp->setChecked(s.value("settings/http_dev_server").toBool());
    ui->checkBoxDebugLog->setChecked(s.value("settings/enable_dev_log").toBool());
    ui->checkBoxBackupNotification->setChecked(s.value("settings/backup_notification", true).toBool());
    ui->spinBoxDefaultPwdLength->setValue(s.value("settings/default_password_length", Common::DEFAULT_PASSWORD_LENGTH).toInt());
#ifdef Q_OS_MAC
    resize(width(), MAC_DEFAULT_HEIGHT);
#endif
}

MainWindow::~MainWindow()
{
    QSettings s;
    s.setValue("MainWindow/geometry", saveGeometry());
    s.setValue("MainWindow/windowState", saveState());

    delete ui;
}

void MainWindow::fillLockUnlockItems()
{
    using LF = Common::LockUnlockModeFeatureFlags;
    m_lockUnlockItems = {
        {tr("Disabled"), (uint)LF::Disabled},
        {tr("Password Only"), (uint)LF::Password},
        {tr("Login + Password"), (uint)LF::Password|(uint)LF::Login},
        {tr("[Enter] + Password"), (uint)LF::Password|(uint)LF::SendEnter},
        {tr("[Ctrl+Alt+Del] + Pass"), (uint)LF::SendCtrl_Alt_Del|(uint)LF::Password},
        {tr("Password / [Win+L]"), (uint)LF::Password|(uint)LF::SendWin_L},
        {tr("Login + Pass / [Win+L]"), (uint)LF::Password|(uint)LF::Login|(uint)LF::SendWin_L},
        {tr("[Enter] + Pass / [Win+L]"), (uint)LF::Password|(uint)LF::SendEnter|(uint)LF::SendWin_L},
        {tr("[Ctrl+Alt+Del] + Pass / [Win+L]"), (uint)LF::SendCtrl_Alt_Del|(uint)LF::Password|(uint)LF::SendWin_L}
    };

    for (auto &item : m_lockUnlockItems)
    {
        ui->lockUnlockModeComboBox->addItem(item.action, item.bitmap|m_noPwdPrompt);
    }

    ui->lockUnlockModeComboBox->setCurrentIndex(0);
}

void MainWindow::updateLockUnlockItems()
{
    for (int i = 0; i < m_lockUnlockItems.size(); ++i)
    {
        ui->lockUnlockModeComboBox->setItemData(i, m_lockUnlockItems[i].bitmap|m_noPwdPrompt);
    }
    int val = ui->lockUnlockModeComboBox->currentIndex();
    // Need to modify selection temporarly to detect change
    ui->lockUnlockModeComboBox->setCurrentIndex(0);
    ui->lockUnlockModeComboBox->setCurrentIndex(val);
}

void MainWindow::checkLockUnlockReset()
{
    bool settingNoPrompt = wsClient->settingsHelper()->getLockUnlockMode()&NO_PWD_PROMPT_MASK;
    bool cbNoPrompt = ui->lockUnlockModeComboBox->currentData().toInt()&NO_PWD_PROMPT_MASK;
    if (settingNoPrompt != cbNoPrompt)
    {
        ui->checkBoxNoPasswordPrompt->setChecked(!ui->checkBoxNoPasswordPrompt->isChecked());
        noPasswordPromptChanged(ui->checkBoxNoPasswordPrompt->isChecked());
    }

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
    if (event->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow())
        {
            emit mainWindowActivated();
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::updateDeviceDependentUI()
{
    wsClient->settingsHelper()->createSettingUIMapping();
    if (wsClient->isMPBLE())
    {
        if (wsClient->get_advancedMenu())
        {
            ui->groupBox_UserSettings->show();
        }
        ui->groupBoxNiMHRecondition->show();
        ui->pbBleBattery->show();
        ui->groupBoxSecurityChallenge->show();
        ui->pushButtonFido->setVisible(true);
        ui->pushButtonSettingsSetToDefault->setVisible(true);
    }
    else
    {
        ui->groupBox_UserSettings->hide();
        ui->groupBoxNiMHRecondition->hide();
        ui->groupBoxSecurityChallenge->hide();
        ui->pushButtonNotes->setVisible(false);
    }
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

void MainWindow::displayBLENameChangedDialog()
{
    QMessageBox::information(this, tr("Device Bluetooth Name Changed"),
                             tr("Please disable and re-enable bluetooth for your changes to take effect"));
}

void MainWindow::updatePage()
{
    const auto status = wsClient->get_status();
    bool isCardUnknown = status == Common::UnknownSmartcard;
    if (wsClient->isMPBLE())
    {
        if (Common::MMMMode == status)
        {
            // Do not update pages when BLE is in MMM Mode
            return;
        }

        if (Common::NoBundle == status)
        {
            bBleDevTabVisible = true;
            ui->pushButtonBleDev->setVisible(bBleDevTabVisible);
            previousWidget = ui->stackedWidget->currentWidget();
            ui->stackedWidget->setCurrentWidget(ui->pageBleDev);
            updateTabButtons();
            return;
        }
    }


    ui->label_13->setVisible(!isCardUnknown);
    ui->label_14->setVisible(!isCardUnknown);
    ui->pushButtonExportFile->setVisible(!isCardUnknown);
    ui->label_exportDBHelp->setVisible(!isCardUnknown);

    const bool integrityVisible = !isCardUnknown;
    ui->label_integrityCheck->setVisible(integrityVisible);
    ui->label_integrityCheckDesc->setVisible(integrityVisible);
    ui->pushButtonIntegrity->setVisible(integrityVisible);
    ui->label_integrityCheckHelp->setVisible(integrityVisible);

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

    else if (ui->pushButtonFido->isChecked())
        ui->stackedWidget->setCurrentWidget(ui->pageFido);

    else if (ui->pushButtonNotes->isChecked())
        ui->stackedWidget->setCurrentWidget(ui->pageNotes);

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
        auto serialNum = QString::number(wsClient->get_hwSerial());
        ui->labelAbouHwSerialValue->setText(wsClient->get_hwSerial() > 0 ? serialNum : NONE_STRING);
        ui->labelAbouHwMemoryValue->setText(wsClient->get_hwMemory() > 0 ? tr("%1Mb").arg(wsClient->get_hwMemory()): NONE_STRING);
        displayMCUVersion(wsClient->isMPBLE());
        //When ble is detected not displaying fw version
        ui->labelAboutFwVers->setVisible(!wsClient->isMPBLE());
        ui->labelAboutFwVersValue->setVisible(!wsClient->isMPBLE());
        ui->label_UserManual->setText(MANUAL_STRING.arg(wsClient->isMPBLE() ? BLE_MANUAL_URL : MINI_MANUAL_URL));
        ui->label_UserManual->show();
        if (wsClient->isMPBLE() && wsClient->get_hwSerial() > STARTING_NOT_FLASHED_SERIAL &&
                wsClient->get_hwSerial() == wsClient->get_platformSerial())
        {
            ui->widgetNotFlashedWarning->show();
        }
        else
        {
            ui->widgetNotFlashedWarning->hide();
        }
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
        wsClient->set_bundleVersion(0);
        wsClient->set_platformSerial(0);
        ui->label_UserManual->hide();
    }
}

void MainWindow::checkSettingsChanged()
{
    bool uichanged = wsClient->settingsHelper()->checkSettingsChanged();
    //lang combobox

    if (uichanged && wsClient->areSettingsFetched())
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
    if (wsClient->isMPBLE())
    {
        checkLockUnlockReset();
    }
    wsClient->settingsHelper()->resetSettings();

    ui->pushButtonSettingsReset->setVisible(false);
    ui->pushButtonSettingsSave->setVisible(false);
}

void MainWindow::on_pushButtonSettingsSave_clicked()
{
    QJsonObject o;
    wsClient->settingsHelper()->getChangedSettings(o);

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
        ui->stackedWidget->setCurrentWidget(previousWidget != nullptr ? previousWidget : ui->pageSettings);
        ui->widgetBleDev->clearWidgets();
    }
}

void MainWindow::onDisplayHiddenTabs()
{
    if ((!wsClient->isMPBLE() || !wsClient->isDeviceConnected()))
    {
        // Only show advanced tab, BLE is not connected
        onAdvancedTabShortcutActivated();
        return;
    }

    if (bBleDevTabVisible && bAdvancedTabVisible)
    {
        ui->stackedWidget->setCurrentWidget(previousWidget != nullptr ? previousWidget : ui->pageSettings);
        bBleDevTabVisible = bAdvancedTabVisible = false;
    }
    else
    {
        previousWidget = ui->stackedWidget->currentWidget();
        bBleDevTabVisible = bAdvancedTabVisible = true;
        ui->stackedWidget->setCurrentWidget(ui->pageAdvanced);
    }
    ui->pushButtonBleDev->setVisible(bBleDevTabVisible);
    ui->pushButtonAdvanced->setVisible(bAdvancedTabVisible);
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
    *conn = connect(wsClient, &WSClient::credentialsUpdated, [this, conn](const QString & , const QString &, const QString &, bool success, const QString& msg)
    {
        disconnect(*conn);
        if (!success)
        {
            QString errorMessage;
            if (Common::MMM_CREDENTIAL_STORE_FAILED == msg)
            {
                errorMessage = tr("Couldn't change all passwords, please approve prompts on the device");
            }
            else
            {
                errorMessage = tr("Couldn't save credentials, please contact the support team with moolticute's log");
            }
            QMessageBox::warning(this, tr("Failure"), errorMessage);
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
    if (ui->tutorialWidget->isTutorialFinished())
    {
        ui->promptWidget->setPromptMessage(message);
        ui->promptWidget->show();
    }
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

void MainWindow::lockUnlockChanged(int val)
{
    bool checked = ui->checkBoxNoPasswordPrompt->isChecked();
    bool noPwdPromptEnabled = val & NO_PWD_PROMPT_MASK;
    if (noPwdPromptEnabled != checked)
    {
        //Update no password prompt checkbox, based on value set on device
        ui->checkBoxNoPasswordPrompt->setChecked(noPwdPromptEnabled);
    }
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

void MainWindow::wantExitFidoManagement()
{
    ui->labelWait->show();
    ui->labelWait->setText(tr("<html><!--exit_fido_mgm_job--><head/><body><p><span style=\"font-size:12pt; font-weight:600;\">Saving changes to device's memory</span></p><p>Please wait.</p></body></html>"));
    ui->stackedWidget->setCurrentWidget(ui->pageWaiting);
    ui->progressBarWait->hide();
    ui->labelProgressMessage->hide();
    updateTabButtons();
}

void MainWindow::wantChangeNote()
{
    ui->labelWait->show();
    ui->labelWait->setText(tr("<html><!--enter_credentials_mgm_job--><head/><body><p><span style=\"font-size:12pt; font-weight:600;\">Waiting For Device Confirmation</span></p><p>Confirm the request on your device.</p></body></html>"));
    ui->stackedWidget->setCurrentWidget(ui->pageWaiting);
    ui->progressBarWait->hide();
    ui->labelProgressMessage->hide();
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

    // SSH keys tab will show up only after activating the SHIFT+F1 shortcut
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

    const int maxsz = 1000 * 1024;

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

bool MainWindow::isDebugLogChecked()
{
    return ui->checkBoxDebugLog->isChecked();
}

bool MainWindow::isBackupNotification() const
{
    return ui->checkBoxBackupNotification->isChecked();
}

void MainWindow::on_checkBoxSSHAgent_stateChanged(int)
{
    QSettings s;
    s.setValue("settings/auto_start_ssh", ui->checkBoxSSHAgent->isChecked());
}

void MainWindow::on_checkBoxTLDCheck_stateChanged(int)
{
    QSettings s;
    s.setValue("settings/disable_tld_check", ui->checkBoxTLDCheck->isChecked()); 
}

void MainWindow::on_pushButtonExportFile_clicked()
{
    wsClient->exportDbFile(Common::SIMPLE_CRYPT);

    // one-time connection, must be disconected immediately in the slot
    connect(wsClient, &WSClient::dbExported, this, &MainWindow::dbExported);

    wantExportDatabase();
}

void MainWindow::on_pushButtonImportFile_clicked()
{
    QSettings s;

    QString fname = AppGui::getFileName(this, tr("Select database export..."),
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
        QString fname = AppGui::getSaveFileName(this, tr("Save database export..."),
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
            {
                QMessageBox::warning(this, tr("Error"), tr("Unable to write to file %1").arg(fname));
            }
            else
            {
                f.write(d);
                QMessageBox::information(this, tr("Moolticute"), tr("Successfully exported the database from your device."));
            }
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
    if (success)
    {
        if (wsClient->isMPBLE())
        {
            wsClient->sendGetUserCategories();
            wsClient->sendFetchNotes();
        }
        QMessageBox::information(this, tr("Moolticute"), tr("Successfully imported and merged database into the device."));
    }
    else
    {
        QMessageBox::warning(this, tr("Error"), message);
    }

    ui->stackedWidget->setCurrentWidget(ui->pageSync);

    updateTabButtons();
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

void MainWindow::setSecurityChallengeText(const QString &id, const QString &bundleVersion)
{
    ui->labelSecurityChallenge->setText(tr("To be sure that no one has tampered with your device, you can request a challenge string and enter it below.<ol>"
                                    "<li>Get the serial number from the back of your device.</li>"
                                    "<li>&shy;<a href=\"mailto:support@themooltipass.com?subject=Security Challenge Token and Response Request&body=My serial number is %1, my bundle number is %2 and my order number is: FILL ME\">Send us an email</a> with the serial number and your order number, requesting the challenge string.</li>"
                                    "<li>Enter the string you received from us</li></ol>").arg(id).arg(bundleVersion));
}

void MainWindow::enableCredentialsManagement(bool enable)
{
    if (enable && ui->stackedWidget->currentWidget() == ui->pageWaiting)
    {
        if (ui->pushButtonCred->isChecked())
            ui->stackedWidget->setCurrentWidget(ui->pageCredentials);
        else if (ui->pushButtonFiles->isChecked())
            ui->stackedWidget->setCurrentWidget(ui->pageFiles);
        else if (ui->pushButtonFido->isChecked())
            ui->stackedWidget->setCurrentWidget(ui->pageFido);
        else if (ui->pushButtonNotes->isChecked())
            ui->stackedWidget->setCurrentWidget(ui->pageNotes);
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

    if (!ui->tutorialWidget->isTutorialFinished())
    {
        setEnabledToAllTabButtons(false);
        ui->tutorialWidget->displayCurrentTab();
        return;
    }

    if (ui->stackedWidget->currentWidget() == ui->pageWaiting)
    {
        setEnabledToAllTabButtons(false);
        return;
    }

    if (wsClient->get_status() == Common::NoBundle)
    {
        setEnabledToAllTabButtons(false);
        return;
    }

    // Enable or Disable tabs according to the device status
    if (wsClient->get_status() == Common::UnknownSmartcard)
    {
        // Enable all tab buttons
        setEnabledToAllTabButtons(true);

        ui->pushButtonCred->setEnabled(false);
        ui->pushButtonFiles->setEnabled(false);
        ui->pushButtonFido->setEnabled(false);
        ui->pushButtonNotes->setEnabled(false);
        ui->pushButtonSSH->setEnabled(false);

        return;
    }

    if (((ui->stackedWidget->currentWidget() == ui->pageFiles
         || ui->stackedWidget->currentWidget() == ui->pageCredentials
         || ui->stackedWidget->currentWidget() == ui->pageIntegrity
         || ui->stackedWidget->currentWidget() == ui->pageFido
         || ui->stackedWidget->currentWidget() == ui->pageNotes) &&
            wsClient->get_memMgmtMode()) ||
            (ui->widgetNotes->isInNoteEditingMode() &&
             ui->stackedWidget->currentWidget() == ui->pageNotes))
    {
        // Disable all tab buttons
        setEnabledToAllTabButtons(false);

        ui->pushButtonCred->setEnabled(ui->stackedWidget->currentWidget() == ui->pageCredentials);
        ui->pushButtonFiles->setEnabled(ui->stackedWidget->currentWidget() == ui->pageFiles);
        ui->pushButtonFiles->setEnabled(ui->stackedWidget->currentWidget() == ui->pageNotes);

        return;
    }

    setEnabledToAllTabButtons(true);
}

void MainWindow::memMgmtModeFailed(int errCode, QString errMsg)
{
    Q_UNUSED(errCode)
    updatePage();
    showPrompt(new PromptMessage{"<b>" + PromptWidget::MMM_ERROR + "</b><br>" +
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

void MainWindow::displayBundleVersion()
{
    if (wsClient->isMPBLE())
    {
        ui->labelBundleVersionValue->setText(QString::number(wsClient->get_bundleVersion()));
        const bool displayBundle = wsClient->get_bundleVersion() > 0;
        ui->labelBundleVersion->setVisible(displayBundle);
        ui->labelBundleVersionValue->setVisible(displayBundle);
        ui->pushButtonNotes->setVisible(wsClient->get_bundleVersion() >= 1);
        if (wsClient->get_bundleVersion() < Common::BLE_LATEST_BUNDLE_VERSION)
        {
            ui->labelBundleOutdatedText->setText(BUNDLE_OUTDATED_TEXT.arg(Common::BLE_LATEST_BUNDLE_VERSION).arg(wsClient->get_hwSerial()).arg(wsClient->get_bundleVersion()));
            ui->labelBundleOutdatedText->show();
            ui->labelBundleOutdatedWarning->show();
            ui->labelBundleOutdatedWarning2->show();
        }
        else
        {
            ui->labelBundleOutdatedText->hide();
            ui->labelBundleOutdatedWarning->hide();
            ui->labelBundleOutdatedWarning2->hide();
        }
    }
    else
    {
        ui->labelBundleVersion->hide();
        ui->labelBundleVersionValue->hide();
        ui->labelBundleOutdatedText->hide();
        ui->labelBundleOutdatedWarning->hide();
        ui->labelBundleOutdatedWarning2->hide();
    }
}

void MainWindow::onBundleVersionChanged(int bundle)
{
    if (wsClient->isMPBLE() && bundle >= Common::BLE_BUNDLE_WITH_BLE_NAME)
    {
        wsClient->sendBleNameRequest();
    }
}

void MainWindow::updateBLEComboboxItems(QComboBox *cb, const QJsonObject& items)
{
    cb->clear();
    for (auto it = items.begin(); it != items.end(); ++it)
    {
        cb->addItem(it.key(), it.value().toInt());
    }
    QSortFilterProxyModel* proxy = new QSortFilterProxyModel(cb);
    proxy->setSourceModel( cb->model());
    cb->model()->setParent(proxy);
    cb->setModel(proxy);
    cb->model()->sort(0);
}

bool MainWindow::shouldUpdateItems(QJsonObject &cache, const QJsonObject &received)
{
    if (cache == received)
    {
        return false;
    }

    cache = received;
    return true;
}

void MainWindow::noPasswordPromptChanged(bool val)
{
    m_noPwdPrompt = val ? NO_PWD_PROMPT_MASK : 0x00;
    updateLockUnlockItems();
}

void MainWindow::handleNoBundleDisconnected()
{
    bBleDevTabVisible = false;
    ui->pushButtonBleDev->setVisible(bBleDevTabVisible);
    if (previousWidget == ui->pageBleDev || nullptr == previousWidget)
    {
        ui->stackedWidget->setCurrentWidget(ui->pageSettings);
    }
    else
    {
        ui->stackedWidget->setCurrentWidget(previousWidget);
    }
    ui->widgetBleDev->clearWidgets();
    wsClient->set_status(Common::UnknownStatus);
    updatePage();
}

void MainWindow::displayMiniImportWarning()
{
    PromptMessage *message = new PromptMessage("<b>"+tr("Import Warning") + "</b><br>" +
                                                  tr("Files were not imported from mini backup."));
    showPrompt(message);
}

void MainWindow::fillBLEBrightnessComboBox(QComboBox *cb)
{
    cb->addItem("10%", 16);
    cb->addItem("25%", 36);
    cb->addItem("50%", 72);
    cb->addItem("75%", 108);
    cb->addItem("100%", 144);
}

void MainWindow::fillInitialCurrentCategories()
{
    QSettings s;
    ui->comboBoxBleCurrentCategory->blockSignals(true);
    ui->comboBoxBleCurrentCategory->clear();
    ui->comboBoxBleCurrentCategory->addItem(tr("Disabled"), 0);
    ui->comboBoxBleCurrentCategory->addItem("1", 1);
    ui->comboBoxBleCurrentCategory->addItem("2", 2);
    ui->comboBoxBleCurrentCategory->addItem("3", 3);
    ui->comboBoxBleCurrentCategory->addItem("4", 4);
    ui->comboBoxBleCurrentCategory->setCurrentIndex(s.value("settings/enforced_category", 0).toInt());
    ui->comboBoxBleCurrentCategory->blockSignals(false);
}

bool MainWindow::validateSerialString(const QString &serialStr, uint &serialNum)
{
    /*
     *  Correct serial string: MOOLTIPXXXXY,
     *  XXXX is the new serial number
     *  Y is a char which is 'A' + (XXXX)%26
     */

    const int serialStartSize = SERIAL_STR_START.size();
    const int serialStringSize = serialStartSize + SERIAL_NUM_LENGTH + 1;
    if (serialStr.size() != serialStringSize)
    {
        qCritical() << "Serial string has an incorrect size";
        return false;
    }

    if (!serialStr.startsWith(SERIAL_STR_START))
    {
        qCritical() << "Serial string is not started with " << SERIAL_STR_START;
        return false;
    }
    QString serialPart = serialStr.mid(serialStartSize, SERIAL_NUM_LENGTH);
    bool ok = false;
    serialNum = serialPart.toUInt(&ok);
    if (!ok)
    {
        qCritical() << "Serial number part is not a number";
        return false;
    }

    int hash = serialStr.toStdString()[serialStringSize - 1] - 'A';
    if (hash != serialNum % Common::ALPHABET_SIZE)
    {
        serialNum = 0;
        qCritical() << "Incorrect hash in serial string";
        return false;
    }
    return true;
}

void MainWindow::displayReconditionWaitScreen(double lastElapsedTime)
{
    QString lastElapsedText = lastElapsedTime > 0 ?
        QString("<p>Recondition restarted. Last elapsed time: %1 seconds (~ %2 mAh)</p><br><p>Need at least %3 seconds for rated %4 mAh capacity</p>")
                .arg(lastElapsedTime).arg(0) : "";
    ui->labelWait->show();
    ui->labelWait->setText(tr("<html><!--nimh_recondition--><head/><body><p><span style=\"font-size:12pt; font-weight:600;\">NiMH Recondition is in progress.</span></p>%1<p>Please wait.</p></body></html>").arg(lastElapsedText));
    ui->stackedWidget->setCurrentWidget(ui->pageWaiting);
    ui->progressBarWait->hide();
    ui->labelProgressMessage->hide();
    updateTabButtons();
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

    QString fname = AppGui::getFileName(this, tr("Select CSV file to import..."),
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
    QList<QString> duplicateCredsWithDiffPwd;

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

        QMutableListIterator<QStringList> it{readData};
        int i = 0;
        while (it.hasNext())
        {
            auto list = it.next();
            // Check every line of CSV file contains only 3 rows
            if (list.size() != 3)
            {
                invalid_lines.append(i+1);
            }
            ++i;
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
    const bool lockOnSleep = ui->checkBoxLockOnSleep->isChecked();
    if (lockOnSleep && wsClient->get_status() == Common::Unlocked)
    {
        qDebug() << "System event. Locking device!";
        wsClient->sendLockDevice();
        m_computerUnlocked = false;
    }
    if (wsClient->isMPBLE() && lockOnSleep)
    {
        wsClient->sendInformLocked();
    }

    // In certain cases it is necessary to tell the system that it can proceed closing the
    // application down. It doesn't have any effect if the application wasn't about to be closed
    // down!
#ifdef Q_OS_MAC
    QTimer::singleShot(2 * 1000, &eventHandler, &SystemEventHandler::readyToTerminate);
#endif
}

void MainWindow::onSystemUnlock()
{
    if (wsClient->isMPBLE())
    {
        qDebug() << "System is unlocked, informing device.";
        wsClient->sendInformUnlocked();
    }
    m_computerUnlocked = true;
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

void MainWindow::on_pushButtonGetAvailableUsers_clicked()
{
    wsClient->requestAvailableUserNumber();
}

void MainWindow::onDeviceConnected()
{
    if (wsClient->isMPBLE())
    {
        if (m_computerUnlocked)
        {
            wsClient->sendInformUnlocked();
        }
        else
        {
            wsClient->sendInformLocked();
        }
        wsClient->sendUserSettingsRequest();
        wsClient->sendBatteryRequest();
    }
    displayBundleVersion();
    updateDeviceDependentUI();
}

void MainWindow::onDeviceDisconnected()
{
    if (wsClient->isMPBLE())
    {
        ui->pbBleBattery->hide();
        ui->label_charging->hide();
        if (wsClient->get_status() == Common::NoBundle)
        {
            handleNoBundleDisconnected();
        }
        ui->pushButtonFido->setVisible(false);
        ui->pushButtonNotes->setVisible(false);
        noPasswordPromptChanged(false);
        ui->pushButtonSettingsSetToDefault->setVisible(false);
        m_notesFetched = false;
        fillInitialCurrentCategories();
    }
    ui->groupBox_UserSettings->hide();
    wsClient->set_cardId("");
    ui->lineEdit_dbBackupFilePath->setText("");
    on_pushButtonSettingsReset_clicked();
}

void MainWindow::on_checkBoxDebugHttp_stateChanged(int)
{
    QSettings s;
    s.setValue("settings/http_dev_server", isHttpDebugChecked());
}

void MainWindow::on_checkBoxDebugLog_stateChanged(int)
{
    QSettings s;
    s.setValue("settings/enable_dev_log", isDebugLogChecked());
}

void MainWindow::handleAdvancedModeChange(bool isEnabled)
{
    if (isEnabled)
    {
        ui->groupBox_UserSettings->show();
    }
    else
    {
        ui->groupBox_UserSettings->hide();
    }
}

void MainWindow::on_spinBoxDefaultPwdLength_valueChanged(int val)
{
    QSettings s;
    s.setValue("settings/default_password_length", val);
}

void MainWindow::on_checkBoxNoPasswordPrompt_stateChanged(int)
{
    noPasswordPromptChanged(ui->checkBoxNoPasswordPrompt->isChecked());
}

void MainWindow::on_pushButtonNiMHRecondition_clicked()
{
    QMessageBox::StandardButton btn =  QMessageBox::information(
                                            this, tr("NiMH Reconditioning"),
                                            tr("This procedure will take around 5 hours, please confirm"),
                                            QMessageBox::Ok | QMessageBox::No);


    if (btn == QMessageBox::Ok)
    {
        wsClient->sendNiMHReconditioning(ui->checkBoxContinueRecondition->isChecked(), ui->exp_capacity->value());
        displayReconditionWaitScreen();
    }
}

void MainWindow::on_pushButtonSecurityValidate_clicked()
{
    const QString DEBUG_START = "FFFFFFFF";
    auto challengeString = ui->lineEditChallengeString->text().toUpper();
    if (DEBUG_START == challengeString.left(DEBUG_START.size()))
    {
        QMessageBox::information(this, tr("Security Challenge"),
                             tr("This token is only used for debugging purposes"));
    }
    ui->labelSecurityChallengeResult->setText(tr("Waiting for Security Challenge result..."));
    ui->labelSecurityChallengeIcon->setMovie(gb_spinner);
    ui->labelSecurityChallengeIcon->setVisible(true);
    ui->labelSecurityChallengeResult->setVisible(true);
    gb_spinner->start();
    wsClient->sendSecurityChallenge(challengeString);
}

void MainWindow::onReconditionFinished(bool success, double dischargeTime, bool restarted)
{
    ui->stackedWidget->setCurrentWidget(ui->pageAdvanced);
    updatePage();
    updateTabButtons();
    if (success)
    {
        if (!restarted)
        {
            QMessageBox::information(this, tr("NiMH Recondition Finished"),
                     tr("Recondition finished in %1 seconds").arg(dischargeTime));
        }
        else
        {
            displayReconditionWaitScreen(dischargeTime);
        }
    }
    else
    {
        QMessageBox::critical(this, tr("NiMH Recondition Error"),
                     tr("Recondition finished with error"));
    }
}

void MainWindow::on_checkBoxEnforceBTLayout_stateChanged(int arg1)
{
    m_keyboardBTLayoutActualValue = Qt::Checked == arg1;
    const auto btLayout = ui->comboBoxBtLayout->currentData().toInt();

    // On Gui start bt layout is not fetched yet (0) so we should not check settings
    if (btLayout != 0)
    {
        checkSettingsChanged();
    }
}

void MainWindow::on_checkBoxEnforceUSBLayout_stateChanged(int arg1)
{
    m_keyboardUsbLayoutActualValue = Qt::Checked == arg1;
    const auto usbLayout = ui->comboBoxUsbLayout->currentData().toInt();

    // On Gui start usb layout is not fetched yet (0) so we should not check settings
    if (usbLayout != 0)
    {
        checkSettingsChanged();
    }
}

void MainWindow::on_pushButtonSettingsSetToDefault_clicked()
{
    QMessageBox::StandardButton btn =  QMessageBox::information(
                                            this, tr("Confirm Reset Settings to Default"),
                                            tr("Do you want to reset every settings value to device defaults?"),
                                            QMessageBox::Yes | QMessageBox::Cancel);

    if (QMessageBox::Yes == btn)
    {
        wsClient->sendJsonData({{ "msg", "reset_default_settings" }});
    }
}

void MainWindow::displayNotePage()
{
    ui->stackedWidget->setCurrentWidget(ui->pageNotes);
    updateTabButtons();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    QMainWindow::keyPressEvent(event);
    if (event->key() == Qt::Key_Shift)
    {
        DeviceDetector::instance().shiftPressed();
    }
    if (event->key() == Qt::Key_Control)
    {
        DeviceDetector::instance().ctrlPressed();
    }
    if (!ui->tutorialWidget->isTutorialFinished() && event->key() == Qt::Key_Escape)
    {
        ui->tutorialWidget->onExitClicked();
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    QMainWindow::keyReleaseEvent(event);
    if (event->key() == Qt::Key_Shift)
    {
        DeviceDetector::instance().shiftReleased();
    }
    if (event->key() == Qt::Key_Control)
    {
        DeviceDetector::instance().ctrlReleased();
    }
}

void MainWindow::on_checkBoxBackupNotification_stateChanged(int)
{
    QSettings s;
    s.setValue("settings/backup_notification", isBackupNotification());
}

void MainWindow::on_checkBoxTutorial_stateChanged(int arg1)
{
    ui->tutorialWidget->changeTutorialFinished(Qt::Checked == arg1);
}

void MainWindow::sendRequestNotes(int bundle)
{
    if (!m_notesFetched && bundle >= Common::BLE_BUNDLE_WITH_NOTES &&
            wsClient->isMPBLE() && wsClient->get_status() == Common::Unlocked)
    {
        m_notesFetched = true;
        wsClient->sendFetchNotes();
    }
}

void MainWindow::on_comboBoxBleCurrentCategory_currentIndexChanged(int index)
{
    QSettings s;
    s.setValue("settings/enforced_category", index);
}

void MainWindow::setCurrentCategoryOptions(const QString &cat1, const QString &cat2, const QString &cat3, const QString &cat4)
{
    if (cat1.isEmpty() && cat2.isEmpty() && cat3.isEmpty() && cat4.isEmpty())
    {
        // When BLE device is connected, but not unlocked daemon is sending categories with empty string
        return;
    }
    QSettings s;
    ui->comboBoxBleCurrentCategory->blockSignals(true);
    ui->comboBoxBleCurrentCategory->clear();
    ui->comboBoxBleCurrentCategory->addItem(tr("Disabled"), 0);
    ui->comboBoxBleCurrentCategory->addItem(cat1, 1);
    ui->comboBoxBleCurrentCategory->addItem(cat2, 2);
    ui->comboBoxBleCurrentCategory->addItem(cat3, 3);
    ui->comboBoxBleCurrentCategory->addItem(cat4, 4);
    ui->comboBoxBleCurrentCategory->setCurrentIndex(s.value("settings/enforced_category", 0).toInt());
    ui->comboBoxBleCurrentCategory->blockSignals(false);
}

void MainWindow::on_lineEditBleName_textEdited(const QString &arg1)
{
    m_bleNameActual = arg1;
}

void MainWindow::onBleNameChanged(const QString &name)
{
    m_bleNameActual = name;
    m_bleNameOriginal = name;
    ui->lineEditBleName->setText(name);
    checkSettingsChanged();
}

void MainWindow::onIncorrectSerialNumberClicked()
{
    bool ok = false;
    QString serialNumStr = QInputDialog::getText(this, tr("Incorrect Serial Number"),
                             tr("It looks like your device serial number doesn't match the one present on your device's case.\n") +
                             tr("Please reach out to support@themooltipass.com for the right code to enter below"),
                             QLineEdit::Normal, "", &ok);
    if (ok)
    {
        uint serialNum = 0;
        if (validateSerialString(serialNumStr, serialNum))
        {
            wsClient->sendSetSerialNumber(serialNum);
        }
        else
        {
            qCritical() << "The entered serial number is incorrect: " << serialNumStr;
            QMessageBox::critical(this, tr("Incorrect Serial Number"),
                                  tr("The entered serial number is incorrect!"));
        }
    }
}

void MainWindow::onSerialNumberChanged(bool success, int serialNumber)
{
    auto title = tr("Serial number change");
    if (success)
    {
        wsClient->set_hwSerial(serialNumber);
        if (wsClient->get_hwSerial() == wsClient->get_platformSerial())
        {
            QMessageBox::information(this, title, tr("Set serial number successfully."));
            ui->widgetNotFlashedWarning->hide();
        }
        else
        {
            QMessageBox::critical(this, title, tr("Serial number still does not match platform serial."));
        }
    }
    else
    {
        QMessageBox::critical(this, title, tr("Set serial number failed."));
    }
}
