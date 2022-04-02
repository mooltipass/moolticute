#ifndef DEVICESETTINGSBLE_H
#define DEVICESETTINGSBLE_H

#include "DeviceSettings.h"

class DeviceSettingsBLE : public DeviceSettings
{
    Q_OBJECT

    //MP BLE only
    QT_SETTINGS_PROPERTY(int, keyboard_usb_layout, 0, MPParams::KEYBOARD_USB_LAYOUT)
    QT_SETTINGS_PROPERTY(int, keyboard_bt_layout, 0, MPParams::KEYBOARD_BT_LAYOUT)
    QT_SETTINGS_PROPERTY(int, device_language, 0, MPParams::DEVICE_LANGUAGE)
    QT_SETTINGS_PROPERTY(int, user_language, 0, MPParams::USER_LANGUAGE)
    QT_SETTINGS_PROPERTY(bool, reserved_ble, false, MPParams::RESERVED_BLE)
    QT_SETTINGS_PROPERTY(bool, prompt_animation, false, MPParams::PROMPT_ANIMATION_PARAM)
    QT_SETTINGS_PROPERTY(bool, bool_animation, false, MPParams::BOOT_ANIMATION_PARAM)
    QT_SETTINGS_PROPERTY(bool, device_lock_usb_disc, false, MPParams::DEVICE_LOCK_USB_DISC)
    QT_SETTINGS_PROPERTY(bool, pin_shown_on_back, false, MPParams::PIN_SHOWN_ON_BACK)
    QT_SETTINGS_PROPERTY(bool, pin_show_on_entry, false, MPParams::PIN_SHOW_ON_ENTRY)
    QT_SETTINGS_PROPERTY(bool, disable_ble_on_card_remove, false, MPParams::DISABLE_BLE_ON_CARD_REMOVE)
    QT_SETTINGS_PROPERTY(bool, disable_ble_on_lock, false, MPParams::DISABLE_BLE_ON_LOCK)
    QT_SETTINGS_PROPERTY(int, nb_20mins_ticks_for_lock, 0, MPParams::NB_20MINS_TICKS_FOR_LOCK)
    QT_SETTINGS_PROPERTY(bool, switch_off_after_usb_disc, false, MPParams::SWITCH_OFF_AFTER_USB_DISC)
    QT_SETTINGS_PROPERTY(int, information_time_delay, 0, MPParams::INFORMATION_TIME_DELAY)
    QT_SETTINGS_PROPERTY(bool, bluetooth_shortcuts, false, MPParams::BLUETOOTH_SHORTCUTS)
    QT_SETTINGS_PROPERTY(int, screen_saver_id, 0, MPParams::SCREEN_SAVER_ID)
    QT_SETTINGS_PROPERTY(bool, display_totp_after_recall, false, MPParams::DISP_TOTP_AFTER_RECALL)
    QT_SETTINGS_PROPERTY(bool, start_last_accessed_service, false, MPParams::START_LAST_ACCESSED_SERVICE)
    QT_SETTINGS_PROPERTY(bool, switch_off_after_bt_disc, false, MPParams::SWITCH_OFF_AFTER_BT_DISC)
    QT_SETTINGS_PROPERTY(bool, mc_subdomain_force_status, false, MPParams::MC_SUBDOMAIN_FORCE_STATUS)
    QT_SETTINGS_PROPERTY(bool, fav_last_used_sorted, false, MPParams::FAV_LAST_USED_SORTED)
    QT_SETTINGS_PROPERTY(int, delay_bef_unlock_login, 0, MPParams::DELAY_BEF_UNLOCK_LOGIN)


public:
    DeviceSettingsBLE(QObject *parent);
    virtual ~DeviceSettingsBLE(){}

    void resetDefaultSettings();

    enum BLESettingsByte
    {
        RESERVED_BYTE = 0,
        RANDOM_PIN_BYTE = 1,
        USER_INTERACTION_TIMEOUT_BYTE = 2,
        ANIMATION_DURING_PROMPT_BYTE = 3,
        DEVICE_DEFAULT_LANG = 4,
        DEFAULT_CHAR_AFTER_LOGIN = 5,
        DEFAULT_CHAR_AFTER_PASS = 6,
        DELAY_BETWEEN_KEY_PRESS = 7,
        BOOT_ANIMATION_BYTE = 8,
        DEVICE_LOCK_USB_BYTE = 10,
        KNOCK_DET_BYTE = 11,
        PIN_SHOWN_ON_BACK_BYTE = 14,
        UNLOCK_FEATURE_BYTE = 15,
        DEVICE_TUTORIAL_BYTE = 16,
        PIN_SHOW_ON_ENTRY_BYTE = 17,
        DISABLE_BLE_ON_CARD_REMOVE = 18,
        DISABLE_BLE_ON_LOCK = 19,
        NB_20MINS_TICKS_FOR_LOCK = 20,
        SWITCH_OFF_AFTER_USB_DISC = 21,
        HASH_DISPLAY_BYTE = 22,
        INFORMATION_TIME_DELAY_BYTE = 23,
        BLUETOOTH_SHORTCUTS_BYTE = 24,
        SCREEN_SAVER_ID_BYTE = 25,
        START_LAST_ACCESSED_SERVICE = 26,
        DISP_TOTP_AFTER_RECALL = 27,
        DELAY_BEF_UNLOCK_LOGIN = 29,
        SWITCH_OFF_AFTER_BT_DISC = 30,
        MC_SUBDOMAIN_FORCE_STATUS = 31,
        FAV_LAST_USED_SORTED = 32
    };

    static constexpr char USB_LAYOUT_ID = 0x01;
    static constexpr char BT_LAYOUT_ID = 0x00;

protected:
    void fillParameterMapping();

    QMap<MPParams::Param, int> m_bleByteMapping;
    using DefaultValues = QMap<MPParams::Param, int>;
    static DefaultValues m_bleDefaultValues;
};

#endif // DEVICESETTINGSBLE_H
