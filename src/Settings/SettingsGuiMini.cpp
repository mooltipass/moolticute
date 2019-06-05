#include "SettingsGuiMini.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

SettingsGuiMini::SettingsGuiMini(QObject *parent, MainWindow *mw)
    : DeviceSettingsMini(parent),
      ISettingsGui(mw)
{
    ui = mw->ui;
}

void SettingsGuiMini::loadParameters()
{
    ui->checkBoxKnock->setChecked(get_knock_enabled());
    ui->checkBoxScreensaver->setChecked(get_screensaver());
    ui->randomStartingPinCheckBox->setChecked(get_random_starting_pin());
    ui->hashDisplayFeatureCheckBox->setChecked(get_hash_display());
    updateComboBoxIndex(ui->lockUnlockModeComboBox, get_lock_unlock_mode());
    updateComboBoxIndex(ui->comboBoxScreenBrightness, get_screen_brightness());
    ui->comboBoxKnock->setCurrentIndex(get_knock_sensitivity());
}

void SettingsGuiMini::updateParam(MPParams::Param param, int val)
{
    setProperty(getParamName(param), val);
}

void SettingsGuiMini::createSettingUIMapping()
{
    ui->groupBox_BLESettings->hide();
}

bool SettingsGuiMini::checkSettingsChanged()
{
    if (ui->checkBoxKnock->isChecked() != get_knock_enabled())
        return true;
    if (ui->comboBoxKnock->currentData().toInt() != get_knock_sensitivity())
        return true;
    if (ui->randomStartingPinCheckBox->isChecked() != get_random_starting_pin())
        return true;
    if (ui->hashDisplayFeatureCheckBox->isChecked() != get_hash_display())
        return true;
    if (ui->lockUnlockModeComboBox->currentData().toInt() != get_lock_unlock_mode())
        return true;
    if (ui->comboBoxScreenBrightness->currentData().toInt() != get_screen_brightness())
        return true;
    return false;
}

void SettingsGuiMini::getChangedSettings(QJsonObject &o, bool isNoCardInsterted)
{
    if (ui->comboBoxScreenBrightness->currentData().toInt() != get_screen_brightness())
        o["screen_brightness"] = ui->comboBoxScreenBrightness->currentData().toInt();
    if (ui->randomStartingPinCheckBox->isChecked() != get_random_starting_pin())
        o["random_starting_pin"] = ui->randomStartingPinCheckBox->isChecked();
    if (ui->hashDisplayFeatureCheckBox->isChecked() != get_hash_display())
        o["hash_display"] = ui->hashDisplayFeatureCheckBox->isChecked();
    if (ui->lockUnlockModeComboBox->currentData().toInt() != get_lock_unlock_mode())
        o["lock_unlock_mode"] = ui->lockUnlockModeComboBox->currentData().toInt();

    if (isNoCardInsterted)
    {
        if (ui->checkBoxKnock->isChecked() != get_knock_enabled())
            o["knock_enabled"] = ui->checkBoxKnock->isChecked();
        if (ui->comboBoxKnock->currentData().toInt() != get_knock_sensitivity())
            o["knock_sensitivity"] = ui->comboBoxKnock->currentData().toInt();
    }
}
