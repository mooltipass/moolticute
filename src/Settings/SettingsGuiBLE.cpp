#include "SettingsGuiBLE.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

SettingsGuiBLE::SettingsGuiBLE(QObject *parent, MainWindow *mw)
    : DeviceSettingsBLE(parent),
      ISettingsGui(mw)
{
    ui = mw->ui;
    connect(this, &DeviceSettings::lock_unlock_modeChanged, mw, &MainWindow::lockUnlockChanged);
}

void SettingsGuiBLE::loadParameters()
{
    qCritical() << "Unimplemented";
}

void SettingsGuiBLE::updateParam(MPParams::Param param, int val)
{
    checkEnforceLayout(param, val);
    setProperty(getParamName(param), val);
}

void SettingsGuiBLE::updateUI()
{
    // Keyboard groupbox
    ui->comboBoxLoginOutput->clear();
    ui->comboBoxLoginOutput->addItem(MainWindow::NONE_STRING, NONE_INDEX);
    ui->comboBoxLoginOutput->addItem(MainWindow::TAB_STRING, TAB_INDEX);
    ui->comboBoxLoginOutput->addItem(MainWindow::ENTER_STRING, ENTER_INDEX);
    ui->comboBoxLoginOutput->addItem(MainWindow::SPACE_STRING, SPACE_INDEX);
    ui->comboBoxPasswordOutput->clear();
    ui->comboBoxPasswordOutput->addItem(MainWindow::NONE_STRING, NONE_INDEX);
    ui->comboBoxPasswordOutput->addItem(MainWindow::TAB_STRING, TAB_INDEX);
    ui->comboBoxPasswordOutput->addItem(MainWindow::ENTER_STRING, ENTER_INDEX);
    ui->comboBoxPasswordOutput->addItem(MainWindow::SPACE_STRING, SPACE_INDEX);
    ui->checkBoxSlowHost->hide();
    ui->checkBoxSendAfterLogin->hide();
    ui->checkBoxSendAfterPassword->hide();
    ui->settings_keyboard_layout->hide();
    ui->settings_usb_layout->show();
    ui->settings_bt_layout->show();
    ui->settings_user_language->show();
    ui->settings_device_language->show();

    // Miscellaneous groupbox
    ui->settings_miscellaneous_brightness->hide();
    ui->checkBoxTuto->show();
    ui->checkBoxBoot->hide();
    ui->checkBoxKnock->hide();
    ui->label_KnockSensitivity->show();
    ui->label_KnockEnable->hide();
    ui->knockSettingsSuffixLabel->hide();
    ui->labelRemoveCard->hide();
    ui->checkBoxFlash->hide();
    ui->checkBoxLockDevice->hide();
    ui->checkBoxPinOnBack->show();
    ui->checkBoxPinOnEntry->show();
    ui->checkBoxNoPasswordPrompt->show();
    ui->checkBoxSwitchOffUSBDisc->show();
    ui->checkBoxDisableBleOnCardRemove->show();
    ui->checkBoxDisableBleOnLock->show();
    ui->settings_information_time_delay->show();
    ui->settings_screen_saver_id->show();
    ui->checkBoxBTShortcuts->show();

    // Bundle 9
    ui->checkBoxDispTOTPAfterRecall->show();
    ui->checkBoxStartWithLastAccessedService->show();
    ui->checkBoxSwitchOffBTDisc->show();
    ui->checkBoxMCSubdomainForceStatus->show();
    ui->checkBoxSortFavsByLastUsed->show();

    ui->groupBox_BLESettings->show();
    ui->checkBoxBLEReserved->hide();

    // Inactivity groupbox
    ui->settings_inactivity_lock->hide();
    ui->checkBoxScreensaver->hide();
    ui->settings_advanced_lockunlock->show();
    ui->setting_ble_inactivity_lock->show();
    ui->checkBoxInput->hide();
}

void SettingsGuiBLE::setupKeyboardLayout(bool onlyCheck)
{
    m_mw->wsClient->requestBleKeyboardLayout(onlyCheck);
}

void SettingsGuiBLE::checkEnforceLayout(MPParams::Param param, int &val)
{
    QSettings s;
    if (param == MPParams::KEYBOARD_USB_LAYOUT && s.value(Common::SETTING_USB_LAYOUT_ENFORCE).toBool())
    {
        val = s.value(Common::SETTING_USB_LAYOUT_ENFORCE_VALUE).toInt();
        qDebug() << "Using usb layout value from enforce: " << val;
    }
    if (param == MPParams::KEYBOARD_BT_LAYOUT && s.value(Common::SETTING_BT_LAYOUT_ENFORCE).toBool())
    {
        val = s.value(Common::SETTING_BT_LAYOUT_ENFORCE_VALUE).toInt();
        qDebug() << "Using bt layout value from enforce: " << val;
    }
}
