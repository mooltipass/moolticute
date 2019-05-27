#ifndef MPSETTINGSMINI_H
#define MPSETTINGSMINI_H

#include "MPSettings.h"

class MPSettingsMini : public MPSettings
{
    Q_OBJECT

    //MP Mini only
    QT_SETTINGS_PROPERTY(int, screenBrightness, 0, MPParams::MINI_OLED_CONTRAST_CURRENT_PARAM) //51-20%, 89-35%, 128-50%, 166-65%, 204-80%, 255-100%
    QT_SETTINGS_PROPERTY(bool, knockEnabled, false, MPParams::MINI_KNOCK_DETECT_ENABLE_PARAM)
    QT_SETTINGS_PROPERTY(int, knockSensitivity, 0, MPParams::MINI_KNOCK_THRES_PARAM) // 0-very low, 1-low, 2-medium, 3-high
    QT_SETTINGS_PROPERTY(bool, randomStartingPin, false, MPParams::RANDOM_INIT_PIN_PARAM)
    QT_SETTINGS_PROPERTY(bool, hashDisplay, false, MPParams::HASH_DISPLAY_FEATURE_PARAM)
    QT_SETTINGS_PROPERTY(int, lockUnlockMode, 0, MPParams::LOCK_UNLOCK_FEATURE_PARAM)

public:
    MPSettingsMini(MPDevice *parent, IMessageProtocol *mesProt);
    virtual ~MPSettingsMini() {}

    void loadParameters() override;
    void updateParam(MPParams::Param param, int val) override;
};

#endif // MPSETTINGSMINI_H