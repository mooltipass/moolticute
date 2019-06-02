#include "SettingsGuiMini.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

SettingsGuiMini::SettingsGuiMini(QObject* parent)
    : DeviceSettingsMini(parent)
{

}

void SettingsGuiMini::updateParam(MPParams::Param param, int val)
{
    setProperty(getParamName(param), val);
}

void SettingsGuiMini::createSettingUIMapping(MainWindow *mw)
{
    connect(this, &SettingsGuiMini::screen_brightnessChanged, [=]()
    {
        updateComboBoxIndex(mw->ui->comboBoxScreenBrightness, get_screen_brightness());
        mw->checkSettingsChanged();
    });
    connect(this, &SettingsGuiMini::knock_enabledChanged, [=]()
    {
        mw->ui->checkBoxKnock->setChecked(get_knock_enabled());
        mw->checkSettingsChanged();
    });
    connect(this, &SettingsGuiMini::knock_sensitivityChanged, [=]()
    {
        mw->ui->comboBoxKnock->setCurrentIndex(get_knock_sensitivity());
        mw->checkSettingsChanged();
    });
    connect(this, &SettingsGuiMini::random_starting_pinChanged, [=]()
    {
        mw->ui->randomStartingPinCheckBox->setChecked(get_random_starting_pin());
        mw->checkSettingsChanged();
    });

    connect(this, &SettingsGuiMini::hash_displayChanged, [=]()
    {
        mw->ui->hashDisplayFeatureCheckBox->setChecked(get_hash_display());
        mw->checkSettingsChanged();
    });

    connect(this, &SettingsGuiMini::lock_unlock_modeChanged, [=]()
    {
        updateComboBoxIndex(mw->ui->lockUnlockModeComboBox, get_lock_unlock_mode());
        mw->checkSettingsChanged();
    });
}

bool SettingsGuiMini::checkSettingsChanged(MainWindow *mw)
{
    if (mw->ui->checkBoxKnock->isChecked() != get_knock_enabled())
        return true;
    if (mw->ui->comboBoxKnock->currentData().toInt() != get_knock_sensitivity())
        return true;
    if (mw->ui->randomStartingPinCheckBox->isChecked() != get_random_starting_pin())
        return true;
    if (mw->ui->hashDisplayFeatureCheckBox->isChecked() != get_hash_display())
        return true;
    if (mw->ui->lockUnlockModeComboBox->currentData().toInt() != get_lock_unlock_mode())
        return true;
    if (mw->ui->comboBoxScreenBrightness->currentData().toInt() != get_screen_brightness())
        return true;
    return false;
}

void SettingsGuiMini::resetSettings(MainWindow *mw)
{
    mw->ui->checkBoxKnock->setChecked(get_knock_enabled());
    mw->ui->checkBoxScreensaver->setChecked(get_screensaver());
    mw->ui->randomStartingPinCheckBox->setChecked(get_random_starting_pin());
    mw->ui->hashDisplayFeatureCheckBox->setChecked(get_hash_display());
    updateComboBoxIndex(mw->ui->lockUnlockModeComboBox, get_lock_unlock_mode());
    updateComboBoxIndex(mw->ui->comboBoxScreenBrightness, get_screen_brightness());
    mw->ui->comboBoxKnock->setCurrentIndex(get_knock_sensitivity());
}

void SettingsGuiMini::getChangedSettings(MainWindow *mw, QJsonObject &o, bool isNoCardInsterted)
{
    if (mw->ui->comboBoxScreenBrightness->currentData().toInt() != get_screen_brightness())
        o["screen_brightness"] = mw->ui->comboBoxScreenBrightness->currentData().toInt();
    if (mw->ui->randomStartingPinCheckBox->isChecked() != get_random_starting_pin())
        o["random_starting_pin"] = mw->ui->randomStartingPinCheckBox->isChecked();
    if (mw->ui->hashDisplayFeatureCheckBox->isChecked() != get_hash_display())
        o["hash_display"] = mw->ui->hashDisplayFeatureCheckBox->isChecked();
    if (mw->ui->lockUnlockModeComboBox->currentData().toInt() != get_lock_unlock_mode())
        o["lock_unlock_mode"] = mw->ui->lockUnlockModeComboBox->currentData().toInt();

    if (isNoCardInsterted)
    {
        if (mw->ui->checkBoxKnock->isChecked() != get_knock_enabled())
            o["knock_enabled"] = mw->ui->checkBoxKnock->isChecked();
        if (mw->ui->comboBoxKnock->currentData().toInt() != get_knock_sensitivity())
            o["knock_sensitivity"] = mw->ui->comboBoxKnock->currentData().toInt();
    }
}

void SettingsGuiMini::loadParameters()
{
    qCritical() << "Not implemented yet";
}
