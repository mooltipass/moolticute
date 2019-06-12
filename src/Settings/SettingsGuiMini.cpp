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
    qCritical() << "Unimplemented";
}

void SettingsGuiMini::updateParam(MPParams::Param param, int val)
{
    setProperty(getParamName(param), val);
}

void SettingsGuiMini::updateUI()
{
    ui->groupBox_BLESettings->hide();
}
