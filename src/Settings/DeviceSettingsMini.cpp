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
    m_paramMap.insert(MPParams::MINI_OLED_CONTRAST_CURRENT_PARAM, "screen_brightness");
    m_paramMap.insert(MPParams::MINI_KNOCK_DETECT_ENABLE_PARAM, "knock_enabled");
    m_paramMap.insert(MPParams::MINI_KNOCK_THRES_PARAM, "knock_sensitivity");
    m_paramMap.insert(MPParams::HASH_DISPLAY_FEATURE_PARAM, "hash_display");
    m_paramMap.insert(MPParams::LOCK_UNLOCK_FEATURE_PARAM, "lock_unlock_mode");
}
