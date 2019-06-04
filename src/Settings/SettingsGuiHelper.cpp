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
    //When something changed in GUI, show save/reset buttons
    connect(ui->comboBoxLang, SIGNAL(currentIndexChanged(int)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxLock, SIGNAL(toggled(bool)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->spinBoxLock, SIGNAL(valueChanged(int)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxScreensaver, SIGNAL(toggled(bool)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxInput, SIGNAL(toggled(bool)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->spinBoxInput, SIGNAL(valueChanged(int)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxFlash, SIGNAL(toggled(bool)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxBoot, SIGNAL(toggled(bool)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxTuto, SIGNAL(toggled(bool)), m_mw, SLOT(checkSettingsChanged()));

    connect(ui->comboBoxScreenBrightness, SIGNAL(currentIndexChanged(int)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxKnock, SIGNAL(toggled(bool)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->comboBoxKnock, SIGNAL(currentIndexChanged(int)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->randomStartingPinCheckBox, SIGNAL(toggled(bool)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->hashDisplayFeatureCheckBox, SIGNAL(toggled(bool)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->lockUnlockModeComboBox, SIGNAL(currentIndexChanged(int)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxSendAfterLogin, SIGNAL(toggled(bool)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->comboBoxLoginOutput, SIGNAL(currentIndexChanged(int)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxSendAfterPassword, SIGNAL(toggled(bool)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->comboBoxPasswordOutput, SIGNAL(currentIndexChanged(int)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxSlowHost, SIGNAL(toggled(bool)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->spinBoxInputDelayAfterKeyPressed, SIGNAL(valueChanged(int)), m_mw, SLOT(checkSettingsChanged()));

    connect(ui->checkBoxBLEReserved, SIGNAL(toggled(bool)), m_mw, SLOT(checkSettingsChanged()));
    connect(ui->checkBoxPromptAnim, SIGNAL(toggled(bool)), m_mw, SLOT(checkSettingsChanged()));
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

    if (!(m_guiSettings = dynamic_cast<ISettingsGui*>(m_settings)))
    {
        return;
    }

    connect(m_settings, &DeviceSettings::keyboard_layoutChanged, [=]()
    {
        updateComboBoxIndex(ui->comboBoxLang, m_settings->get_keyboard_layout());
        m_mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::lock_timeout_enabledChanged, [=]()
    {
        ui->checkBoxLock->setChecked(m_settings->get_lock_timeout_enabled());
        m_mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::lock_timeoutChanged, [=]()
    {
        ui->spinBoxLock->setValue(m_settings->get_lock_timeout());
        m_mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::screensaverChanged, [=]()
    {
        ui->checkBoxScreensaver->setChecked(m_settings->get_screensaver());
        m_mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::user_request_cancelChanged, [=]()
    {
        ui->checkBoxInput->setChecked(m_settings->get_user_request_cancel());
        m_mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::user_interaction_timeoutChanged, [=]()
    {
        ui->spinBoxInput->setValue(m_settings->get_user_interaction_timeout());
        m_mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::flash_screenChanged, [=]()
    {
        ui->checkBoxFlash->setChecked(m_settings->get_flash_screen());
        m_mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::offline_modeChanged, [=]()
    {
        ui->checkBoxBoot->setChecked(m_settings->get_offline_mode());
        m_mw->checkSettingsChanged();
    });
    connect(m_settings, &DeviceSettings::tutorial_enabledChanged, [=]()
    {
        ui->checkBoxTuto->setChecked(m_settings->get_tutorial_enabled());
        m_mw->checkSettingsChanged();
    });

    connect(m_settings, &DeviceSettings::key_after_login_enabledChanged, [=]()
    {
        ui->checkBoxSendAfterLogin->setChecked(m_settings->get_key_after_login_enabled());
        m_mw->checkSettingsChanged();
    });

    connect(m_settings, &DeviceSettings::key_after_loginChanged, [=]()
    {
        updateComboBoxIndex(ui->comboBoxLoginOutput, m_settings->get_key_after_login());
        m_mw->checkSettingsChanged();
    });

    connect(m_settings, &DeviceSettings::key_after_pass_enabledChanged, [=]()
    {
        ui->checkBoxSendAfterPassword->setChecked(m_settings->get_key_after_pass_enabled());
        m_mw->checkSettingsChanged();
    });

    connect(m_settings, &DeviceSettings::key_after_passChanged, [=]()
    {
        updateComboBoxIndex(ui->comboBoxPasswordOutput, m_settings->get_key_after_pass());
        m_mw->checkSettingsChanged();
    });

    connect(m_settings, &DeviceSettings::delay_after_key_enabledChanged, [=]()
    {
        ui->checkBoxSlowHost->setChecked(m_settings->get_delay_after_key_enabled());
        m_mw->checkSettingsChanged();
    });

    connect(m_settings, &DeviceSettings::delay_after_keyChanged, [=]()
    {
        ui->spinBoxInputDelayAfterKeyPressed->setValue(m_settings->get_delay_after_key());
        m_mw->checkSettingsChanged();
    });

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

    m_guiSettings->checkSettingsChanged();

    return false;
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
