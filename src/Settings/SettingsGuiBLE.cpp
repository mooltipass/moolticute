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
    ui->groupBox_keyboard->hide();
    ui->groupBox_miscellaneous->hide();
    ui->groupBox_BLESettings->show();
    ui->checkBoxBLEReserved->hide();

    ui->settings_inactivity_lock->hide();
    ui->checkBoxScreensaver->hide();
    ui->hashDisplayFeatureCheckBox->hide();
    ui->settings_advanced_lockunlock->hide();
}
