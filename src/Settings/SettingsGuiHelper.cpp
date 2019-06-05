#include "SettingsGuiHelper.h"
#include "SettingsGuiMini.h"
#include "SettingsGuiBLE.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"


SettingsGuiHelper::SettingsGuiHelper(WSClient* parent)
    : QObject(parent),
      m_wsClient{parent}
{
}

void SettingsGuiHelper::setMainWindow(MainWindow *mw)
{
    m_mw = mw;
    ui = mw->ui;
    m_widgetMapping = {
        {MPParams::KEYBOARD_LAYOUT_PARAM, ui->comboBoxLang},
        {MPParams::LOCK_TIMEOUT_ENABLE_PARAM, ui->checkBoxLock},
        {MPParams::LOCK_TIMEOUT_PARAM, ui->spinBoxLock},
        {MPParams::SCREENSAVER_PARAM, ui->checkBoxScreensaver},
        {MPParams::USER_REQ_CANCEL_PARAM, ui->checkBoxInput},
        {MPParams::USER_INTER_TIMEOUT_PARAM, ui->spinBoxInput},
        {MPParams::FLASH_SCREEN_PARAM, ui->checkBoxFlash},
        {MPParams::OFFLINE_MODE_PARAM, ui->checkBoxBoot},
        {MPParams::TUTORIAL_BOOL_PARAM, ui->checkBoxTuto},
        {MPParams::KEY_AFTER_LOGIN_SEND_BOOL_PARAM, ui->checkBoxSendAfterLogin},
        {MPParams::KEY_AFTER_LOGIN_SEND_PARAM, ui->comboBoxLoginOutput},
        {MPParams::KEY_AFTER_PASS_SEND_BOOL_PARAM, ui->checkBoxSendAfterPassword},
        {MPParams::KEY_AFTER_PASS_SEND_PARAM, ui->comboBoxPasswordOutput},
        {MPParams::DELAY_AFTER_KEY_ENTRY_BOOL_PARAM, ui->checkBoxSlowHost},
        {MPParams::DELAY_AFTER_KEY_ENTRY_PARAM, ui->spinBoxInputDelayAfterKeyPressed},
        {MPParams::MINI_OLED_CONTRAST_CURRENT_PARAM, ui->comboBoxScreenBrightness},
        {MPParams::RANDOM_INIT_PIN_PARAM, ui->randomStartingPinCheckBox},
        {MPParams::MINI_KNOCK_DETECT_ENABLE_PARAM, ui->checkBoxKnock},
        {MPParams::MINI_KNOCK_THRES_PARAM, ui->comboBoxKnock},
        {MPParams::HASH_DISPLAY_FEATURE_PARAM, ui->hashDisplayFeatureCheckBox},
        {MPParams::LOCK_UNLOCK_FEATURE_PARAM, ui->lockUnlockModeComboBox},
        {MPParams::RESERVED_BLE, ui->checkBoxBLEReserved},
        {MPParams::PROMPT_ANIMATION_PARAM, ui->checkBoxPromptAnim},
        {MPParams::PROMPT_ANIMATION_PARAM, ui->checkBoxLockDevice}
    };
    //When something changed in GUI, show save/reset buttons
    for (const auto& widget : m_widgetMapping)
    {
        std::string signal;
        if (dynamic_cast<QComboBox*>(widget))
        {
            signal = SIGNAL(currentIndexChanged(int));
        } else if (dynamic_cast<QCheckBox*>(widget))
        {
            signal = SIGNAL(toggled(bool));
        } else if (dynamic_cast<QSpinBox*>(widget))
        {
            signal = SIGNAL(valueChanged(int));
        }
        connect(widget, signal.c_str(), m_mw, SLOT(checkSettingsChanged()));
    }
}

void SettingsGuiHelper::createSettingUIMapping()
{
    const auto type = m_wsClient->get_mpHwVersion();
    if (type != m_deviceType)
    {
        delete m_settings;
    }
    m_deviceType = type;
    if (m_deviceType == Common::MP_BLE)
    {
        m_settings = new SettingsGuiBLE(this, m_mw);
    }
    else if (m_deviceType == Common::MP_Mini || m_deviceType == Common::MP_Classic)
    {
        m_settings = new SettingsGuiMini(this, m_mw);
    }
    else
    {
        return;
    }

    m_guiSettings = dynamic_cast<ISettingsGui*>(m_settings);
    m_settings->connectSendParams(this);
    m_guiSettings->createSettingUIMapping();
}

bool SettingsGuiHelper::checkSettingsChanged()
{
    if (ui->comboBoxLang->currentData().toInt() != m_settings->get_keyboard_layout())
        return true;
    if (ui->checkBoxLock->isChecked() != m_settings->get_lock_timeout_enabled())
        return true;
    if (ui->spinBoxLock->value() != m_settings->get_lock_timeout())
        return true;
    if (ui->checkBoxScreensaver->isChecked() != m_settings->get_screensaver())
        return true;
    if (ui->checkBoxInput->isChecked() != m_settings->get_user_request_cancel())
        return true;
    if (ui->spinBoxInput->value() != m_settings->get_user_interaction_timeout())
        return true;
    if (ui->checkBoxFlash->isChecked() != m_settings->get_flash_screen())
        return true;
    if (ui->checkBoxBoot->isChecked() != m_settings->get_offline_mode())
        return true;
    if (ui->checkBoxTuto->isChecked() != m_settings->get_tutorial_enabled())
        return true;
    if (ui->checkBoxSendAfterLogin->isChecked() != m_settings->get_key_after_login_enabled())
        return true;
    if (ui->comboBoxLoginOutput->currentData().toInt() != m_settings->get_key_after_login())
        return true;
    if (ui->checkBoxSendAfterPassword->isChecked() != m_settings->get_key_after_pass_enabled())
        return true;
    if (ui->comboBoxPasswordOutput->currentData().toInt() != m_settings->get_key_after_pass())
        return true;
    if (ui->checkBoxSlowHost->isChecked() != m_settings->get_delay_after_key_enabled())
        return true;
    if (ui->spinBoxInputDelayAfterKeyPressed->value() != m_settings->get_delay_after_key())
        return true;

    return m_guiSettings->checkSettingsChanged();
}

void SettingsGuiHelper::resetSettings()
{
    ui->checkBoxLock->setChecked(m_settings->get_lock_timeout_enabled());
    ui->spinBoxLock->setValue(m_settings->get_lock_timeout());
    ui->checkBoxInput->setChecked(m_settings->get_user_request_cancel());
    ui->spinBoxInput->setValue(m_settings->get_user_interaction_timeout());
    ui->checkBoxFlash->setChecked(m_settings->get_flash_screen());
    ui->checkBoxBoot->setChecked(m_settings->get_offline_mode());
    ui->checkBoxTuto->setChecked(m_settings->get_tutorial_enabled());
    ui->checkBoxSendAfterLogin->setChecked(m_settings->get_key_after_login_enabled());
    ui->checkBoxSendAfterPassword->setChecked(m_settings->get_key_after_pass_enabled());
    ui->checkBoxSlowHost->setChecked(m_settings->get_delay_after_key_enabled());
    ui->spinBoxInputDelayAfterKeyPressed->setValue(m_settings->get_delay_after_key());

    updateComboBoxIndex(ui->comboBoxLoginOutput, m_settings->get_key_after_login());
    updateComboBoxIndex(ui->comboBoxPasswordOutput, m_settings->get_key_after_pass_enabled());
    updateComboBoxIndex(ui->comboBoxLang, m_settings->get_keyboard_layout());

    m_settings->loadParameters();
}

void SettingsGuiHelper::getChangedSettings(QJsonObject &o)
{
    if (ui->comboBoxLang->currentData().toInt() != m_settings->get_keyboard_layout())
        o["keyboard_layout"] = ui->comboBoxLang->currentData().toInt();
    if (ui->checkBoxLock->isChecked() != m_settings->get_lock_timeout_enabled())
        o["lock_timeout_enabled"] = ui->checkBoxLock->isChecked();
    if (ui->spinBoxLock->value() != m_settings->get_lock_timeout())
        o["lock_timeout"] = ui->spinBoxLock->value();
    if (ui->checkBoxScreensaver->isChecked() != m_settings->get_screensaver())
        o["screensaver"] = ui->checkBoxScreensaver->isChecked();
    if (ui->checkBoxInput->isChecked() != m_settings->get_user_request_cancel())
        o["user_request_cancel"] = ui->checkBoxInput->isChecked();
    if (ui->spinBoxInput->value() != m_settings->get_user_interaction_timeout())
        o["user_interaction_timeout"] = ui->spinBoxInput->value();
    if (ui->checkBoxFlash->isChecked() != m_settings->get_flash_screen())
        o["flash_screen"] = ui->checkBoxFlash->isChecked();
    if (ui->checkBoxBoot->isChecked() != m_settings->get_offline_mode())
        o["offline_mode"] = ui->checkBoxBoot->isChecked();
    if (ui->checkBoxTuto->isChecked() != m_settings->get_tutorial_enabled())
        o["tutorial_enabled"] = ui->checkBoxTuto->isChecked();
    if (ui->checkBoxSendAfterLogin->isChecked() != m_settings->get_key_after_login_enabled())
        o["key_after_login_enabled"] = ui->checkBoxSendAfterLogin->isChecked();
    if (ui->comboBoxLoginOutput->currentData().toInt() != m_settings->get_key_after_login())
        o["key_after_login"] = ui->comboBoxLoginOutput->currentData().toInt();
    if (ui->checkBoxSendAfterPassword->isChecked() != m_settings->get_key_after_pass_enabled())
        o.insert("key_after_pass_enabled", ui->checkBoxSendAfterPassword->isChecked());
    if (ui->comboBoxPasswordOutput->currentData().toInt() != m_settings->get_key_after_pass())
        o["key_after_pass"] = ui->comboBoxPasswordOutput->currentData().toInt();
    if (ui->checkBoxSlowHost->isChecked() != m_settings->get_delay_after_key_enabled())
        o["delay_after_key_enabled"] = ui->checkBoxSlowHost->isChecked();
    if (ui->spinBoxInputDelayAfterKeyPressed->value() != m_settings->get_delay_after_key())
        o["delay_after_key"] = ui->spinBoxInputDelayAfterKeyPressed->value();

    m_guiSettings->getChangedSettings(o, m_wsClient->get_status() == Common::NoCardInserted);
}

void SettingsGuiHelper::updateParameters(const QJsonObject &data)
{
    QString param = data["parameter"].toString();
    int val = data["value"].toInt();
    if (data["value"].isBool())
    {
        val = data["value"].toBool();
    }
    m_settings->updateParam(m_settings->getParamId(param), val);
}

void SettingsGuiHelper::sendParams(bool value, int param)
{
    QWidget* widget = m_widgetMapping[MPParams::Param(param)];
    if (auto* comboBox = dynamic_cast<QCheckBox*>(widget))
    {
        comboBox->setChecked(value);
    }
    else
    {
        qCritical() << "Invalid widget";
    }
    m_mw->checkSettingsChanged();
}

void SettingsGuiHelper::sendParams(int value, int param)
{
    QWidget* widget = m_widgetMapping[MPParams::Param(param)];
    if (auto* comboBox = dynamic_cast<QComboBox*>(widget))
    {
        updateComboBoxIndex(comboBox, value);
    }
    else if (auto* spinBox = dynamic_cast<QSpinBox*>(widget))
    {
        spinBox->setValue(value);
    }
    else
    {
        qCritical() << "Invalid widget";
    }
    m_mw->checkSettingsChanged();
}
