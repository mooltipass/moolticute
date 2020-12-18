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
    m_paramMap.insert(MPParams::DEVICE_LOCK_USB_DISC, "device_lock_usb_disc");
    m_bleByteMapping[MPParams::DEVICE_LOCK_USB_DISC] = DEVICE_LOCK_USB_BYTE;
    m_bleByteMapping[MPParams::MINI_KNOCK_THRES_PARAM] = KNOCK_DET_BYTE;
    m_bleByteMapping[MPParams::LOCK_UNLOCK_FEATURE_PARAM] = UNLOCK_FEATURE_BYTE;
    m_paramMap.insert(MPParams::PIN_SHOWN_ON_BACK, "pin_shown_on_back");
    m_bleByteMapping[MPParams::PIN_SHOWN_ON_BACK] = PIN_SHOWN_ON_BACK_BYTE;
    m_paramMap.insert(MPParams::PIN_SHOW_ON_ENTRY, "pin_show_on_entry");
    m_bleByteMapping[MPParams::TUTORIAL_BOOL_PARAM] = DEVICE_TUTORIAL_BYTE;
    m_bleByteMapping[MPParams::PIN_SHOW_ON_ENTRY] = PIN_SHOW_ON_ENTRY_BYTE;
    m_paramMap.insert(MPParams::DISABLE_BLE_ON_CARD_REMOVE, "disable_ble_on_card_remove");
    m_bleByteMapping[MPParams::DISABLE_BLE_ON_CARD_REMOVE] = DISABLE_BLE_ON_CARD_REMOVE;
    m_paramMap.insert(MPParams::DISABLE_BLE_ON_LOCK, "disable_ble_on_lock");
    m_bleByteMapping[MPParams::DISABLE_BLE_ON_LOCK] = DISABLE_BLE_ON_LOCK;
    m_paramMap.insert(MPParams::NB_20MINS_TICKS_FOR_LOCK, "nb_20mins_ticks_for_lock");
    m_bleByteMapping[MPParams::NB_20MINS_TICKS_FOR_LOCK] = NB_20MINS_TICKS_FOR_LOCK;
    m_paramMap.insert(MPParams::SWITCH_OFF_AFTER_USB_DISC, "switch_off_after_usb_disc");
    m_bleByteMapping[MPParams::SWITCH_OFF_AFTER_USB_DISC] = SWITCH_OFF_AFTER_USB_DISC;
    m_bleByteMapping[MPParams::HASH_DISPLAY_FEATURE_PARAM] = HASH_DISPLAY_BYTE;
    m_paramMap.insert(MPParams::INFORMATION_TIME_DELAY, "information_time_delay");
    m_bleByteMapping[MPParams::INFORMATION_TIME_DELAY] = INFORMATION_TIME_DELAY_BYTE;
}

