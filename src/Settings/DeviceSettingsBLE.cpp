#include "DeviceSettingsBLE.h"

DeviceSettingsBLE::DeviceSettingsBLE(QObject *parent)
    : DeviceSettings(parent)
{
    fillParameterMapping();
}

void DeviceSettingsBLE::fillParameterMapping()
{
    m_paramMap.insert(MPParams::KEYBOARD_USB_LAYOUT, "keyboard_usb_layout");
    m_paramMap.insert(MPParams::KEYBOARD_BT_LAYOUT, "keyboard_bt_layout");
    m_paramMap.insert(MPParams::DEVICE_LANGUAGE, "device_language");
    m_paramMap.insert(MPParams::USER_LANGUAGE, "user_language");
    m_bleByteMapping[MPParams::RANDOM_INIT_PIN_PARAM] = RANDOM_PIN_BYTE;
    m_bleByteMapping[MPParams::USER_INTER_TIMEOUT_PARAM] = USER_INTERACTION_TIMEOUT_BYTE;
    m_bleByteMapping[MPParams::KEY_AFTER_LOGIN_SEND_PARAM] = DEFAULT_CHAR_AFTER_LOGIN;
    m_bleByteMapping[MPParams::KEY_AFTER_PASS_SEND_PARAM] = DEFAULT_CHAR_AFTER_PASS;
    m_bleByteMapping[MPParams::DELAY_AFTER_KEY_ENTRY_PARAM] = DELAY_BETWEEN_KEY_PRESS;
    m_paramMap.insert(MPParams::RESERVED_BLE, "reserved_ble");
    m_bleByteMapping[MPParams::RESERVED_BLE] = RESERVED_BYTE;
    m_paramMap.insert(MPParams::PROMPT_ANIMATION_PARAM, "prompt_animation");
    m_bleByteMapping[MPParams::PROMPT_ANIMATION_PARAM] = ANIMATION_DURING_PROMPT_BYTE;
    m_paramMap.insert(MPParams::BOOT_ANIMATION_PARAM, "bool_animation");
    m_bleByteMapping[MPParams::BOOT_ANIMATION_PARAM] = BOOT_ANIMATION_BYTE;
}

