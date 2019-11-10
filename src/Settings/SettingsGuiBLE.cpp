#include "SettingsGuiBLE.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

SettingsGuiBLE::SettingsGuiBLE(QObject *parent, MainWindow *mw)
    : DeviceSettingsBLE(parent),
      ISettingsGui(mw)
{
    ui = mw->ui;
}

void SettingsGuiBLE::loadParameters()
{
    qCritical() << "Unimplemented";
}

void SettingsGuiBLE::updateParam(MPParams::Param param, int val)
{
    setProperty(getParamName(param), val);
}

void SettingsGuiBLE::updateUI()
{
    // Keyboard groupbox
    ui->comboBoxLoginOutput->clear();
    ui->comboBoxLoginOutput->addItem(tr("None"), NONE_INDEX);
    ui->comboBoxLoginOutput->addItem(tr("Tab"), TAB_INDEX);
    ui->comboBoxLoginOutput->addItem(tr("Enter"), ENTER_INDEX);
    ui->comboBoxLoginOutput->addItem(tr("Space"), SPACE_INDEX);
    ui->comboBoxPasswordOutput->clear();
    ui->comboBoxPasswordOutput->addItem(tr("None"), NONE_INDEX);
    ui->comboBoxPasswordOutput->addItem(tr("Tab"), TAB_INDEX);
    ui->comboBoxPasswordOutput->addItem(tr("Enter"), ENTER_INDEX);
    ui->comboBoxPasswordOutput->addItem(tr("Space"), SPACE_INDEX);
    ui->checkBoxSlowHost->hide();
    ui->checkBoxSendAfterLogin->hide();
    ui->checkBoxSendAfterPassword->hide();
    ui->settings_keyboard_layout->hide();
    ui->settings_usb_layout->show();
    ui->settings_bt_layout->show();
    ui->settings_user_language->show();
    ui->settings_device_language->show();

    // Miscellaneous groupbox
    ui->groupBox_miscellaneous->hide();

    ui->groupBox_BLESettings->show();

    // Inactivity groupbox
    ui->settings_inactivity_lock->hide();
    ui->checkBoxScreensaver->hide();
    ui->hashDisplayFeatureCheckBox->hide();
    ui->settings_advanced_lockunlock->hide();
}

void SettingsGuiBLE::setupKeyboardLayout()
{
    qCritical() << "Setting up keyboard layout";
}
