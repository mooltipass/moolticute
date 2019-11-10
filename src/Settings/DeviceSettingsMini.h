#ifndef DEVICESETTINGSMINI_H
#define DEVICESETTINGSMINI_H

#include <QObject>
#include "DeviceSettings.h"

class DeviceSettingsMini : public DeviceSettings
{
    Q_OBJECT

    QT_SETTINGS_PROPERTY(int, keyboard_layout, 0, MPParams::KEYBOARD_LAYOUT_PARAM)
    QT_SETTINGS_PROPERTY(bool, lock_timeout_enabled, false, MPParams::LOCK_TIMEOUT_ENABLE_PARAM)
    QT_SETTINGS_PROPERTY(int, lock_timeout, 0, MPParams::LOCK_TIMEOUT_PARAM)
    QT_SETTINGS_PROPERTY(bool, screensaver, false, MPParams::SCREENSAVER_PARAM)
    QT_SETTINGS_PROPERTY(bool, user_request_cancel, false, MPParams::USER_REQ_CANCEL_PARAM)
    QT_SETTINGS_PROPERTY(bool, flash_screen, false, MPParams::FLASH_SCREEN_PARAM)
    QT_SETTINGS_PROPERTY(bool, offline_mode, false, MPParams::OFFLINE_MODE_PARAM)
    QT_SETTINGS_PROPERTY(bool, tutorial_enabled, false, MPParams::TUTORIAL_BOOL_PARAM)

    QT_SETTINGS_PROPERTY(bool, key_after_login_enabled, false, MPParams::KEY_AFTER_LOGIN_SEND_BOOL_PARAM)
    QT_SETTINGS_PROPERTY(bool, key_after_pass_enabled, false, MPParams::KEY_AFTER_PASS_SEND_BOOL_PARAM)
    QT_SETTINGS_PROPERTY(bool, delay_after_key_enabled, false, MPParams::DELAY_AFTER_KEY_ENTRY_BOOL_PARAM)

    //MP Mini only
    QT_SETTINGS_PROPERTY(int, screen_brightness, 0, MPParams::MINI_OLED_CONTRAST_CURRENT_PARAM) //51-20%, 89-35%, 128-50%, 166-65%, 204-80%, 255-100%
    QT_SETTINGS_PROPERTY(bool, knock_enabled, false, MPParams::MINI_KNOCK_DETECT_ENABLE_PARAM)
    QT_SETTINGS_PROPERTY(int, knock_sensitivity, 0, MPParams::MINI_KNOCK_THRES_PARAM) // 0-very low, 1-low, 2-medium, 3-high
    QT_SETTINGS_PROPERTY(bool, hash_display, false, MPParams::HASH_DISPLAY_FEATURE_PARAM)
    QT_SETTINGS_PROPERTY(int, lock_unlock_mode, 0, MPParams::LOCK_UNLOCK_FEATURE_PARAM)

public:
    DeviceSettingsMini(QObject *parent);
    virtual ~DeviceSettingsMini() {}

protected:
    void convertKnockValue(int& val);
    void checkTimeoutBoundaries(int& val);
    void fillParameterMapping();
};

#endif // DEVICESETTINGSMINI_H
