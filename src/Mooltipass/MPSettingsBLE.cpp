#include "MPSettingsBLE.h"
#include "AsyncJobs.h"
#include "MPDevice.h"
#include "IMessageProtocol.h"
#include "WSServerCon.h"

MPSettingsBLE::MPSettingsBLE(MPDevice *parent, IMessageProtocol *mesProt)
    : DeviceSettingsBLE(parent),
      mpDevice{parent},
      pMesProt{mesProt}
{
}

void MPSettingsBLE::loadParameters()
{
    m_readingParams = true;
    AsyncJobs *jobs = new AsyncJobs(
                          "Loading device parameters",
                          this);

    jobs->append(new MPCommandJob(mpDevice,
                   MPCmd::GET_DEVICE_SETTINGS,
                   [this] (const QByteArray &data, bool &)
                    {
                        m_lastDeviceSettings = pMesProt->getFullPayload(data);
                        qDebug() << "Full device settings payload: " << m_lastDeviceSettings.toHex();
                        set_reserved_ble(m_lastDeviceSettings.at(DeviceSettingsBLE::RESERVED_BYTE) != 0);
                        set_user_interaction_timeout(m_lastDeviceSettings.at(DeviceSettingsBLE::USER_INTERACTION_TIMEOUT_BYTE));
                        set_random_starting_pin(m_lastDeviceSettings.at(DeviceSettingsBLE::RANDOM_PIN_BYTE) != 0);
                        set_prompt_animation(m_lastDeviceSettings.at(DeviceSettingsBLE::ANIMATION_DURING_PROMPT_BYTE) != 0);
                        set_key_after_login(m_lastDeviceSettings.at(DeviceSettingsBLE::DEFAULT_CHAR_AFTER_LOGIN));
                        set_key_after_pass(m_lastDeviceSettings.at(DeviceSettingsBLE::DEFAULT_CHAR_AFTER_PASS));
                        set_delay_after_key(m_lastDeviceSettings.at(DeviceSettingsBLE::DELAY_BETWEEN_KEY_PRESS));
                        set_bool_animation(m_lastDeviceSettings.at(DeviceSettingsBLE::BOOT_ANIMATION_BYTE) != 0);
                        set_device_lock_usb_disc(m_lastDeviceSettings.at(DeviceSettingsBLE::DEVICE_LOCK_USB_BYTE) != 0);
                        const int knockValue = convertBackKnockValue(m_lastDeviceSettings.at(DeviceSettingsBLE::KNOCK_DET_BYTE));
                        set_knock_sensitivity(knockValue);
                        set_lock_unlock_mode(m_lastDeviceSettings.at(DeviceSettingsBLE::UNLOCK_FEATURE_BYTE));
                        return true;
                    }
    ));
    jobs->append(new MPCommandJob(mpDevice,
                   MPCmd::GET_USER_LANG,
                   [this] (const QByteArray &data, bool &)
                    {
                        const auto langId = pMesProt->getFirstPayloadByte(data);
                        qDebug() << "User language: " << langId;
                        set_user_language(langId);
                        return true;
                    }
    ));
    jobs->append(new MPCommandJob(mpDevice,
                   MPCmd::GET_DEVICE_LANG,
                   [this] (const QByteArray &data, bool &)
                    {
                        const auto langId = pMesProt->getFirstPayloadByte(data);
                        qDebug() << "Device language: " << langId;
                        set_device_language(langId);
                        return true;
                    }
    ));
    jobs->append(new MPCommandJob(mpDevice,
                   MPCmd::GET_KEYB_LAYOUT_ID,
                   QByteArray(1, USB_LAYOUT_ID),
                   [this] (const QByteArray &data, bool &)
                    {
                        const auto layoutId = pMesProt->getFirstPayloadByte(data);
                        qDebug() << "Keyboard USB layout: " << layoutId;
                        set_keyboard_usb_layout(layoutId);
                        return true;
                    }
    ));
    jobs->append(new MPCommandJob(mpDevice,
                   MPCmd::GET_KEYB_LAYOUT_ID,
                   QByteArray(1, BT_LAYOUT_ID),
                   [this] (const QByteArray &data, bool &)
                    {
                        const auto layoutId = pMesProt->getFirstPayloadByte(data);
                        qDebug() << "Keyboard Bluetooth layout: " << layoutId;
                        set_keyboard_bt_layout(layoutId);
                        return true;
                    }
    ));

    mpDevice->enqueueAndRunJob(jobs);
    return;
}

void MPSettingsBLE::updateParam(MPParams::Param param, int val)
{
    if (m_bleByteMapping.contains(param))
    {
        if (MPParams::MINI_KNOCK_THRES_PARAM == param)
        {
            convertKnockValue(val);
        }
        m_lastDeviceSettings[m_bleByteMapping[param]] = static_cast<char>(val);
    }
    else
    {
        setProperty(m_paramMap[param], val);
    }
}

void MPSettingsBLE::connectSendParams(QObject *slotObject)
{
    DeviceSettings::connectSendParams(slotObject);
    connect(slotObject, SIGNAL(parameterProcessFinished()), this, SLOT(setSettings()));
}

void MPSettingsBLE::setSettings()
{
    AsyncJobs *jobs = new AsyncJobs(
                          "Setting device parameters",
                          this);

    jobs->append(new MPCommandJob(mpDevice,
                   MPCmd::SET_DEVICE_SETTINGS,
                   m_lastDeviceSettings,
                   [this] (const QByteArray &data, bool &)
                    {
                        if (0x01 == pMesProt->getFirstPayloadByte(data))
                        {
                            qDebug() << "Set device settings was successfull";
                        }
                        else
                        {
                            qWarning() << "Set device settings failed";
                        }
                        return true;
                    }
    ));
    jobs->append(new MPCommandJob(mpDevice,
                   MPCmd::SET_USER_LANG,
                   QByteArray(1, get_user_language()),
                   pMesProt->getDefaultFuncDone()
    ));
    jobs->append(new MPCommandJob(mpDevice,
                   MPCmd::SET_DEVICE_LANG,
                   QByteArray(1, get_device_language()),
                   pMesProt->getDefaultFuncDone()
    ));
    QByteArray layoutUsbData;
    layoutUsbData.append(USB_LAYOUT_ID);
    layoutUsbData.append(get_keyboard_usb_layout());
    jobs->append(new MPCommandJob(mpDevice,
                   MPCmd::SET_KEYB_LAYOUT_ID,
                   layoutUsbData,
                   pMesProt->getDefaultFuncDone()
    ));
    QByteArray layoutBtData;
    layoutBtData.append(BT_LAYOUT_ID);
    layoutBtData.append(get_keyboard_bt_layout());
    jobs->append(new MPCommandJob(mpDevice,
                   MPCmd::SET_KEYB_LAYOUT_ID,
                   layoutBtData,
                   pMesProt->getDefaultFuncDone()
    ));

    mpDevice->enqueueAndRunJob(jobs);
}


