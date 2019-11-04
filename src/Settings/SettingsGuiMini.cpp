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
    // Keyboard groupbox
    ui->comboBoxLoginOutput->clear();
    ui->comboBoxLoginOutput->addItem(tr("Tab"), TAB_INDEX);
    ui->comboBoxLoginOutput->addItem(tr("Enter"), ENTER_INDEX);
    ui->comboBoxLoginOutput->addItem(tr("Space"), SPACE_INDEX);
    ui->comboBoxPasswordOutput->clear();
    ui->comboBoxPasswordOutput->addItem(tr("Tab"), TAB_INDEX);
    ui->comboBoxPasswordOutput->addItem(tr("Enter"), ENTER_INDEX);
    ui->comboBoxPasswordOutput->addItem(tr("Space"), SPACE_INDEX);
    ui->checkBoxSlowHost->show();
    ui->checkBoxSendAfterLogin->show();
    ui->checkBoxSendAfterPassword->show();
    ui->settings_user_language->hide();
    ui->settings_device_language->hide();

    // Miscellaneous groupbox
    ui->groupBox_miscellaneous->show();

    ui->groupBox_BLESettings->hide();

    // Inactivity groupbox
    ui->settings_inactivity_lock->show();
    ui->checkBoxScreensaver->show();
    ui->hashDisplayFeatureCheckBox->show();
    ui->settings_advanced_lockunlock->show();
}
