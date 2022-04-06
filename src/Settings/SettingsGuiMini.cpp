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
    ui->comboBoxLoginOutput->addItem(MainWindow::TAB_STRING, TAB_INDEX);
    ui->comboBoxLoginOutput->addItem(MainWindow::ENTER_STRING, ENTER_INDEX);
    ui->comboBoxLoginOutput->addItem(MainWindow::SPACE_STRING, SPACE_INDEX);
    ui->comboBoxPasswordOutput->clear();
    ui->comboBoxPasswordOutput->addItem(MainWindow::TAB_STRING, TAB_INDEX);
    ui->comboBoxPasswordOutput->addItem(MainWindow::ENTER_STRING, ENTER_INDEX);
    ui->comboBoxPasswordOutput->addItem(MainWindow::SPACE_STRING, SPACE_INDEX);
    ui->checkBoxSlowHost->show();
    ui->checkBoxSendAfterLogin->show();
    ui->checkBoxSendAfterPassword->show();
    ui->settings_keyboard_layout->show();
    ui->settings_usb_layout->hide();
    ui->settings_bt_layout->hide();
    ui->settings_user_language->hide();
    ui->settings_device_language->hide();

    // Miscellaneous groupbox
    ui->settings_miscellaneous_brightness->show();
    ui->checkBoxTuto->show();
    ui->checkBoxBoot->show();
    ui->checkBoxKnock->show();
    ui->label_KnockSensitivity->hide();
    ui->label_KnockEnable->show();
    ui->knockSettingsSuffixLabel->show();
    ui->labelRemoveCard->show();
    ui->checkBoxFlash->show();
    ui->checkBoxLockDevice->show();
    ui->checkBoxPinOnBack->hide();
    ui->checkBoxPinOnEntry->hide();
    ui->checkBoxNoPasswordPrompt->hide();

    ui->checkBoxSwitchOffUSBDisc->hide();
    ui->checkBoxDisableBleOnCardRemove->hide();
    ui->checkBoxDisableBleOnLock->hide();

    ui->settings_information_time_delay->hide();
    ui->settings_screen_saver_id->hide();
    ui->checkBoxBTShortcuts->hide();

    //Bundle 9
    ui->checkBoxDispTOTPAfterRecall->hide();
    ui->checkBoxStartWithLastAccessedService->hide();
    ui->checkBoxSwitchOffBTDisc->hide();
    ui->checkBoxMCSubdomainForceStatus->hide();
    ui->checkBoxSortFavsByLastUsed->hide();
    ui->settings_delay_bef_unlock_login->hide();
    ui->settings_screen_brightness_bat->hide();
    ui->settings_screen_brightness_usb->hide();

    ui->groupBox_BLESettings->hide();

    // Inactivity groupbox
    ui->settings_inactivity_lock->show();
    ui->checkBoxScreensaver->show();
    ui->settings_advanced_lockunlock->show();
    ui->setting_ble_inactivity_lock->hide();
    ui->checkBoxInput->show();
}

void SettingsGuiMini::setupKeyboardLayout(bool)
{
    auto* langCb = ui->comboBoxLang;
    if (langCb->count() > 0)
    {
        //Combobox for Mini layouts is filled
        return;
    }
    langCb->addItem("en_US", ID_KEYB_EN_US_LUT);
    langCb->addItem("fr_FR", ID_KEYB_FR_FR_LUT);
    langCb->addItem("es_ES", ID_KEYB_ES_ES_LUT);
    langCb->addItem("de_DE", ID_KEYB_DE_DE_LUT);
    langCb->addItem("es_AR", ID_KEYB_ES_AR_LUT);
    langCb->addItem("en_AU", ID_KEYB_EN_AU_LUT);
    langCb->addItem("fr_BE", ID_KEYB_FR_BE_LUT);
    langCb->addItem("po_BR", ID_KEYB_PO_BR_LUT);
    langCb->addItem("en_CA", ID_KEYB_EN_CA_LUT);
    langCb->addItem("cz_CZ", ID_KEYB_CZ_CZ_LUT);
    langCb->addItem("da_DK", ID_KEYB_DA_DK_LUT);
    langCb->addItem("fi_FI", ID_KEYB_FI_FI_LUT);
    langCb->addItem("hu_HU", ID_KEYB_HU_HU_LUT);
    langCb->addItem("is_IS", ID_KEYB_IS_IS_LUT);
    langCb->addItem("it_IT", ID_KEYB_IT_IT_LUT);
    langCb->addItem("nl_NL", ID_KEYB_NL_NL_LUT);
    langCb->addItem("no_NO", ID_KEYB_NO_NO_LUT);
    langCb->addItem("po_PO", ID_KEYB_PO_PO_LUT);
    langCb->addItem("ro_RO", ID_KEYB_RO_RO_LUT);
    langCb->addItem("sl_SL", ID_KEYB_SL_SL_LUT);
    langCb->addItem("frde_CH", ID_KEYB_FRDE_CH_LUT);
    langCb->addItem("en_UK", ID_KEYB_EN_UK_LUT);
    langCb->addItem("cs_QWERTY", ID_KEYB_CZ_QWERTY_LUT);
    langCb->addItem("en_DV", ID_KEYB_EN_DV_LUT);
    langCb->addItem("fr_MAC", ID_KEYB_FR_MAC_LUT);
    langCb->addItem("fr_CH_MAC", ID_KEYB_FR_CH_MAC_LUT);
    langCb->addItem("de_CH_MAC", ID_KEYB_DE_CH_MAC_LUT);
    langCb->addItem("de_MAC", ID_KEYB_DE_MAC_LUT);
    langCb->addItem("us_MAC", ID_KEYB_US_MAC_LUT);

    QSortFilterProxyModel* proxy = new QSortFilterProxyModel(langCb);
    proxy->setSourceModel( langCb->model());
    langCb->model()->setParent(proxy);
    langCb->setModel(proxy);
    langCb->model()->sort(0);
}
