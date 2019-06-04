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
    ui->checkBoxBLEReserved->setChecked(get_reserved_ble());
    ui->checkBoxPromptAnim->setChecked(get_prompt_animation());
}

void SettingsGuiBLE::updateParam(MPParams::Param param, int val)
{
    setProperty(getParamName(param), val);
}

void SettingsGuiBLE::createSettingUIMapping()
{
    ui->groupBox_BLESettings->show();
    connect(this, &SettingsGuiBLE::reserved_bleChanged, [=]()
    {
        ui->checkBoxBLEReserved->setChecked(get_reserved_ble());
        m_mw->checkSettingsChanged();
    });
    connect(this, &SettingsGuiBLE::prompt_animationChanged, [=]()
    {
        ui->checkBoxPromptAnim->setChecked(get_prompt_animation());
        m_mw->checkSettingsChanged();
    });
}

bool SettingsGuiBLE::checkSettingsChanged()
{
    if (ui->checkBoxBLEReserved->isChecked() != get_reserved_ble())
        return true;
    if (ui->checkBoxPromptAnim->isChecked() != get_prompt_animation())
        return true;
    return false;
}

void SettingsGuiBLE::getChangedSettings(QJsonObject &o, bool isNoCardInsterted)
{
    Q_UNUSED(isNoCardInsterted)
    if (ui->checkBoxBLEReserved->isChecked() != get_reserved_ble())
        o["reserved_ble"] = ui->checkBoxBLEReserved->isChecked();
    if (ui->checkBoxPromptAnim->isChecked() != get_prompt_animation())
        o["prompt_animation"] = ui->checkBoxPromptAnim->isChecked();
}
