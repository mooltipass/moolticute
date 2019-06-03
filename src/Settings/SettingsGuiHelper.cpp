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

void SettingsGuiHelper::createSettingUIMapping(MainWindow *mw)
{
    m_mw = mw;
    const auto type = m_wsClient->get_mpHwVersion();
    if (type != m_deviceType)
    {
        delete m_settings;
    }
    m_deviceType = type;
    if (m_deviceType == Common::MP_BLE)
    {
        m_settings = new SettingsGuiBLE(this);
    }
    else if (m_deviceType == Common::MP_Mini || m_deviceType == Common::MP_Classic)
    {
        m_settings = new SettingsGuiMini(this);
    }
    else
    {
        return;
    }

    connect(m_settings, &DeviceSettings::keyboard_layoutChanged, [=]()
    {
        updateComboBoxIndex(mw->ui->comboBoxLang, m_settings->get_keyboard_layout());
        mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::lock_timeout_enabledChanged, [=]()
    {
        mw->ui->checkBoxLock->setChecked(m_settings->get_lock_timeout_enabled());
        mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::lock_timeoutChanged, [=]()
    {
        mw->ui->spinBoxLock->setValue(m_settings->get_lock_timeout());
        mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::screensaverChanged, [=]()
    {
        mw->ui->checkBoxScreensaver->setChecked(m_settings->get_screensaver());
        mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::user_request_cancelChanged, [=]()
    {
        mw->ui->checkBoxInput->setChecked(m_settings->get_user_request_cancel());
        mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::user_interaction_timeoutChanged, [=]()
    {
        mw->ui->spinBoxInput->setValue(m_settings->get_user_interaction_timeout());
        mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::flash_screenChanged, [=]()
    {
        mw->ui->checkBoxFlash->setChecked(m_settings->get_flash_screen());
        mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::offline_modeChanged, [=]()
    {
        mw->ui->checkBoxBoot->setChecked(m_settings->get_offline_mode());
        mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::tutorial_enabledChanged, [=]()
    {
        mw->ui->checkBoxTuto->setChecked(m_settings->get_tutorial_enabled());
        mw->checkSettingsChanged();
    });

    connect(m_settings, &DeviceSettings::key_after_login_enabledChanged, [=]()
    {
        mw->ui->checkBoxSendAfterLogin->setChecked(m_settings->get_key_after_login_enabled());
        mw->checkSettingsChanged();
    });

    connect(m_settings, &DeviceSettings::key_after_loginChanged, [=]()
    {

        updateComboBoxIndex(mw->ui->comboBoxLoginOutput, m_settings->get_key_after_login());
        mw->checkSettingsChanged();
    });

    connect(m_settings, &DeviceSettings::key_after_pass_enabledChanged, [=]()
    {
        mw->ui->checkBoxSendAfterPassword->setChecked(m_settings->get_key_after_pass_enabled());
        mw->checkSettingsChanged();
    });

    connect(m_settings, &DeviceSettings::key_after_passChanged, [=]()
    {
        updateComboBoxIndex(mw->ui->comboBoxPasswordOutput, m_settings->get_key_after_pass());
        mw->checkSettingsChanged();
    });

    connect(m_settings, &DeviceSettings::delay_after_key_enabledChanged, [=]()
    {
        mw->ui->checkBoxSlowHost->setChecked(m_settings->get_delay_after_key_enabled());
        mw->checkSettingsChanged();
    });

    connect(m_settings, &DeviceSettings::delay_after_keyChanged, [=]()
    {
        mw->ui->spinBoxInputDelayAfterKeyPressed->setValue(m_settings->get_delay_after_key());
        mw->checkSettingsChanged();
    });

    if (auto* set = dynamic_cast<SettingsGuiMini*>(m_settings))
    {
        set->createSettingUIMapping(mw);
    }
    else if (auto* set = dynamic_cast<SettingsGuiBLE*>(m_settings))
    {
        set->createSettingUIMapping(mw);
    }
}

bool SettingsGuiHelper::checkSettingsChanged()
{
    if (m_mw->ui->comboBoxLang->currentData().toInt() != m_settings->get_keyboard_layout())
        return true;
    if (m_mw->ui->checkBoxLock->isChecked() != m_settings->get_lock_timeout_enabled())
        return true;
    if (m_mw->ui->spinBoxLock->value() != m_settings->get_lock_timeout())
        return true;
    if (m_mw->ui->checkBoxScreensaver->isChecked() != m_settings->get_screensaver())
        return true;
    if (m_mw->ui->checkBoxInput->isChecked() != m_settings->get_user_request_cancel())
        return true;
    if (m_mw->ui->spinBoxInput->value() != m_settings->get_user_interaction_timeout())
        return true;
    if (m_mw->ui->checkBoxFlash->isChecked() != m_settings->get_flash_screen())
        return true;
    if (m_mw->ui->checkBoxBoot->isChecked() != m_settings->get_offline_mode())
        return true;
    if (m_mw->ui->checkBoxTuto->isChecked() != m_settings->get_tutorial_enabled())
        return true;
    if (m_mw->ui->checkBoxSendAfterLogin->isChecked() != m_settings->get_key_after_login_enabled())
        return true;
    if (m_mw->ui->comboBoxLoginOutput->currentData().toInt() != m_settings->get_key_after_login())
        return true;
    if (m_mw->ui->checkBoxSendAfterPassword->isChecked() != m_settings->get_key_after_pass_enabled())
        return true;
    if (m_mw->ui->comboBoxPasswordOutput->currentData().toInt() != m_settings->get_key_after_pass())
        return true;
    if (m_mw->ui->checkBoxSlowHost->isChecked() != m_settings->get_delay_after_key_enabled())
        return true;
    if (m_mw->ui->spinBoxInputDelayAfterKeyPressed->value() != m_settings->get_delay_after_key())
        return true;

    if (auto* set = dynamic_cast<SettingsGuiMini*>(m_settings))
    {
        return set->checkSettingsChanged();
    }
    else if (auto* set = dynamic_cast<SettingsGuiBLE*>(m_settings))
    {
        set->checkSettingsChanged();
    }

    return false;
}

void SettingsGuiHelper::resetSettings()
{
    m_mw->ui->checkBoxLock->setChecked(m_settings->get_lock_timeout_enabled());
    m_mw->ui->spinBoxLock->setValue(m_settings->get_lock_timeout());
    m_mw->ui->checkBoxInput->setChecked(m_settings->get_user_request_cancel());
    m_mw->ui->spinBoxInput->setValue(m_settings->get_user_interaction_timeout());
    m_mw->ui->checkBoxFlash->setChecked(m_settings->get_flash_screen());
    m_mw->ui->checkBoxBoot->setChecked(m_settings->get_offline_mode());
    m_mw->ui->checkBoxTuto->setChecked(m_settings->get_tutorial_enabled());
    m_mw->ui->checkBoxSendAfterLogin->setChecked(m_settings->get_key_after_login_enabled());
    m_mw->ui->checkBoxSendAfterPassword->setChecked(m_settings->get_key_after_pass_enabled());
    m_mw->ui->checkBoxSlowHost->setChecked(m_settings->get_delay_after_key_enabled());
    m_mw->ui->spinBoxInputDelayAfterKeyPressed->setValue(m_settings->get_delay_after_key());

    updateComboBoxIndex(m_mw->ui->comboBoxLoginOutput, m_settings->get_key_after_login());
    updateComboBoxIndex(m_mw->ui->comboBoxPasswordOutput, m_settings->get_key_after_pass_enabled());
    updateComboBoxIndex(m_mw->ui->comboBoxLang, m_settings->get_keyboard_layout());

    m_settings->loadParameters();
}

void SettingsGuiHelper::getChangedSettings(QJsonObject &o)
{
    if (m_mw->ui->comboBoxLang->currentData().toInt() != m_settings->get_keyboard_layout())
        o["keyboard_layout"] = m_mw->ui->comboBoxLang->currentData().toInt();
    if (m_mw->ui->checkBoxLock->isChecked() != m_settings->get_lock_timeout_enabled())
        o["lock_timeout_enabled"] = m_mw->ui->checkBoxLock->isChecked();
    if (m_mw->ui->spinBoxLock->value() != m_settings->get_lock_timeout())
        o["lock_timeout"] = m_mw->ui->spinBoxLock->value();
    if (m_mw->ui->checkBoxScreensaver->isChecked() != m_settings->get_screensaver())
        o["screensaver"] = m_mw->ui->checkBoxScreensaver->isChecked();
    if (m_mw->ui->checkBoxInput->isChecked() != m_settings->get_user_request_cancel())
        o["user_request_cancel"] = m_mw->ui->checkBoxInput->isChecked();
    if (m_mw->ui->spinBoxInput->value() != m_settings->get_user_interaction_timeout())
        o["user_interaction_timeout"] = m_mw->ui->spinBoxInput->value();
    if (m_mw->ui->checkBoxFlash->isChecked() != m_settings->get_flash_screen())
        o["flash_screen"] = m_mw->ui->checkBoxFlash->isChecked();
    if (m_mw->ui->checkBoxBoot->isChecked() != m_settings->get_offline_mode())
        o["offline_mode"] = m_mw->ui->checkBoxBoot->isChecked();
    if (m_mw->ui->checkBoxTuto->isChecked() != m_settings->get_tutorial_enabled())
        o["tutorial_enabled"] = m_mw->ui->checkBoxTuto->isChecked();
    if (m_mw->ui->checkBoxSendAfterLogin->isChecked() != m_settings->get_key_after_login_enabled())
        o["key_after_login_enabled"] = m_mw->ui->checkBoxSendAfterLogin->isChecked();
    if (m_mw->ui->comboBoxLoginOutput->currentData().toInt() != m_settings->get_key_after_login())
        o["key_after_login"] = m_mw->ui->comboBoxLoginOutput->currentData().toInt();
    if (m_mw->ui->checkBoxSendAfterPassword->isChecked() != m_settings->get_key_after_pass_enabled())
        o.insert("key_after_pass_enabled", m_mw->ui->checkBoxSendAfterPassword->isChecked());
    if (m_mw->ui->comboBoxPasswordOutput->currentData().toInt() != m_settings->get_key_after_pass())
        o["key_after_pass"] = m_mw->ui->comboBoxPasswordOutput->currentData().toInt();
    if (m_mw->ui->checkBoxSlowHost->isChecked() != m_settings->get_delay_after_key_enabled())
        o["delay_after_key_enabled"] = m_mw->ui->checkBoxSlowHost->isChecked();
    if (m_mw->ui->spinBoxInputDelayAfterKeyPressed->value() != m_settings->get_delay_after_key())
        o["delay_after_key"] = m_mw->ui->spinBoxInputDelayAfterKeyPressed->value();

    if (auto* set = dynamic_cast<SettingsGuiMini*>(m_settings))
    {
        set->getChangedSettings(o, m_wsClient->get_status() == Common::NoCardInserted);
    }
    else if (auto* set = dynamic_cast<SettingsGuiBLE*>(m_settings))
    {
        set->getChangedSettings(o);
    }
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
