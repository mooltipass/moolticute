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

public:
    DeviceSettingsBLE(QObject *parent);
    virtual ~DeviceSettingsBLE(){}

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
        KNOCK_DET_BYTE = 11
    };

    static constexpr char USB_LAYOUT_ID = 0x01;
    static constexpr char BT_LAYOUT_ID = 0x00;

protected:
    void fillParameterMapping();
    QMap<MPParams::Param, int> m_bleByteMapping;
};

#endif // DEVICESETTINGSBLE_H
