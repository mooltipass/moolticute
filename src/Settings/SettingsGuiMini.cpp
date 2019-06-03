#include "SettingsGuiMini.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

SettingsGuiMini::SettingsGuiMini(QObject* parent)
    : DeviceSettingsMini(parent)
{

}

void SettingsGuiMini::loadParameters()
{
    m_mw->ui->checkBoxKnock->setChecked(get_knock_enabled());
    m_mw->ui->checkBoxScreensaver->setChecked(get_screensaver());
    m_mw->ui->randomStartingPinCheckBox->setChecked(get_random_starting_pin());
    m_mw->ui->hashDisplayFeatureCheckBox->setChecked(get_hash_display());
    updateComboBoxIndex(m_mw->ui->lockUnlockModeComboBox, get_lock_unlock_mode());
    updateComboBoxIndex(m_mw->ui->comboBoxScreenBrightness, get_screen_brightness());
    m_mw->ui->comboBoxKnock->setCurrentIndex(get_knock_sensitivity());
}

void SettingsGuiMini::updateParam(MPParams::Param param, int val)
{
    setProperty(getParamName(param), val);
}

void SettingsGuiMini::createSettingUIMapping(MainWindow *mw)
{
    m_mw = mw;
    m_mw->ui->groupBox_BLESettings->hide();
    connect(this, &SettingsGuiMini::screen_brightnessChanged, [=]()
    {
        updateComboBoxIndex(m_mw->ui->comboBoxScreenBrightness, get_screen_brightness());
        m_mw->checkSettingsChanged();
    });
    connect(this, &SettingsGuiMini::knock_enabledChanged, [=]()
    {
        m_mw->ui->checkBoxKnock->setChecked(get_knock_enabled());
        m_mw->checkSettingsChanged();
    });
    connect(this, &SettingsGuiMini::knock_sensitivityChanged, [=]()
    {
        m_mw->ui->comboBoxKnock->setCurrentIndex(get_knock_sensitivity());
        m_mw->checkSettingsChanged();
    });
    connect(this, &SettingsGuiMini::random_starting_pinChanged, [=]()
    {
        m_mw->ui->randomStartingPinCheckBox->setChecked(get_random_starting_pin());
        m_mw->checkSettingsChanged();
    });

    connect(this, &SettingsGuiMini::hash_displayChanged, [=]()
    {
        m_mw->ui->hashDisplayFeatureCheckBox->setChecked(get_hash_display());
        m_mw->checkSettingsChanged();
    });

    connect(this, &SettingsGuiMini::lock_unlock_modeChanged, [=]()
    {
        updateComboBoxIndex(m_mw->ui->lockUnlockModeComboBox, get_lock_unlock_mode());
        m_mw->checkSettingsChanged();
    });
}

bool SettingsGuiMini::checkSettingsChanged()
{
    if (m_mw->ui->checkBoxKnock->isChecked() != get_knock_enabled())
        return true;
    if (m_mw->ui->comboBoxKnock->currentData().toInt() != get_knock_sensitivity())
        return true;
    if (m_mw->ui->randomStartingPinCheckBox->isChecked() != get_random_starting_pin())
        return true;
    if (m_mw->ui->hashDisplayFeatureCheckBox->isChecked() != get_hash_display())
        return true;
    if (m_mw->ui->lockUnlockModeComboBox->currentData().toInt() != get_lock_unlock_mode())
        return true;
    if (m_mw->ui->comboBoxScreenBrightness->currentData().toInt() != get_screen_brightness())
        return true;
    return false;
}

void SettingsGuiMini::getChangedSettings(QJsonObject &o, bool isNoCardInsterted)
{
    if (m_mw->ui->comboBoxScreenBrightness->currentData().toInt() != get_screen_brightness())
        o["screen_brightness"] = m_mw->ui->comboBoxScreenBrightness->currentData().toInt();
    if (m_mw->ui->randomStartingPinCheckBox->isChecked() != get_random_starting_pin())
        o["random_starting_pin"] = m_mw->ui->randomStartingPinCheckBox->isChecked();
    if (m_mw->ui->hashDisplayFeatureCheckBox->isChecked() != get_hash_display())
        o["hash_display"] = m_mw->ui->hashDisplayFeatureCheckBox->isChecked();
    if (m_mw->ui->lockUnlockModeComboBox->currentData().toInt() != get_lock_unlock_mode())
        o["lock_unlock_mode"] = m_mw->ui->lockUnlockModeComboBox->currentData().toInt();

    if (isNoCardInsterted)
    {
        if (m_mw->ui->checkBoxKnock->isChecked() != get_knock_enabled())
            o["knock_enabled"] = m_mw->ui->checkBoxKnock->isChecked();
        if (m_mw->ui->comboBoxKnock->currentData().toInt() != get_knock_sensitivity())
            o["knock_sensitivity"] = m_mw->ui->comboBoxKnock->currentData().toInt();
    }
}
