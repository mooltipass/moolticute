#include "DeviceSettingsMini.h"

DeviceSettingsMini::DeviceSettingsMini(QObject *parent)
    : DeviceSettings(parent)
{
    fillParameterMapping();
}

void DeviceSettingsMini::convertKnockValue(int &val)
{
    switch(val)
    {
    case 0: val = KNOCKING_VERY_LOW; break;
    case 1: val = KNOCKING_LOW; break;
    case 2: val = KNOCKING_MEDIUM; break;
    case 3: val = KNOCKING_HIGH; break;
    default:
        val = KNOCKING_MEDIUM;
    }
}

void DeviceSettingsMini::checkTimeoutBoundaries(int &val)
{
    if (val < 0) val = 0;
    if (val > 0xFF) val = 0xFF;
}

void DeviceSettingsMini::fillParameterMapping()
{
    m_paramMap.insert(MPParams::KEYBOARD_LAYOUT_PARAM, "keyboard_layout");
    m_paramMap.insert(MPParams::LOCK_TIMEOUT_ENABLE_PARAM, "lock_timeout_enabled");
    m_paramMap.insert(MPParams::LOCK_TIMEOUT_PARAM, "lock_timeout");
    m_paramMap.insert(MPParams::SCREENSAVER_PARAM, "screensaver");
    m_paramMap.insert(MPParams::USER_REQ_CANCEL_PARAM, "user_request_cancel");
    m_paramMap.insert(MPParams::FLASH_SCREEN_PARAM, "flash_screen");
    m_paramMap.insert(MPParams::OFFLINE_MODE_PARAM, "offline_mode");
    m_paramMap.insert(MPParams::TUTORIAL_BOOL_PARAM, "tutorial_enabled");
    m_paramMap.insert(MPParams::KEY_AFTER_LOGIN_SEND_BOOL_PARAM, "key_after_login_enabled");
    m_paramMap.insert(MPParams::KEY_AFTER_PASS_SEND_BOOL_PARAM, "key_after_pass_enabled");
    m_paramMap.insert(MPParams::DELAY_AFTER_KEY_ENTRY_BOOL_PARAM, "delay_after_key_enabled");
    m_paramMap.insert(MPParams::MINI_OLED_CONTRAST_CURRENT_PARAM, "screen_brightness");
    m_paramMap.insert(MPParams::MINI_KNOCK_DETECT_ENABLE_PARAM, "knock_enabled");
    m_paramMap.insert(MPParams::HASH_DISPLAY_FEATURE_PARAM, "hash_display");
    m_paramMap.insert(MPParams::LOCK_UNLOCK_FEATURE_PARAM, "lock_unlock_mode");
}
