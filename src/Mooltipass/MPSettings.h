#ifndef MPSETTINGS_H
#define MPSETTINGS_H

#include <QObject>
#include "QtHelper.h"
#include "Common.h"
#include "MooltipassCmds.h"

class MPDevice;
class IMessageProtocol;

class MPSettings : public QObject
{
    Q_OBJECT

    QT_SETTINGS_PROPERTY(int, keyboardLayout, 0, MPParams::KEYBOARD_LAYOUT_PARAM)
    QT_SETTINGS_PROPERTY(bool, lockTimeoutEnabled, false, MPParams::LOCK_TIMEOUT_ENABLE_PARAM)
    QT_SETTINGS_PROPERTY(int, lockTimeout, 0, MPParams::LOCK_TIMEOUT_PARAM)
    QT_SETTINGS_PROPERTY(bool, screensaver, false, MPParams::SCREENSAVER_PARAM)
    QT_SETTINGS_PROPERTY(bool, userRequestCancel, false, MPParams::USER_REQ_CANCEL_PARAM)
    QT_SETTINGS_PROPERTY(int, userInteractionTimeout, 0, MPParams::USER_INTER_TIMEOUT_PARAM)
    QT_SETTINGS_PROPERTY(bool, flashScreen, false, MPParams::FLASH_SCREEN_PARAM)
    QT_SETTINGS_PROPERTY(bool, offlineMode, false, MPParams::OFFLINE_MODE_PARAM)
    QT_SETTINGS_PROPERTY(bool, tutorialEnabled, false, MPParams::TUTORIAL_BOOL_PARAM)
    QT_SETTINGS_PROPERTY(int, flashMbSize, 0, MPParams::FLASH_SCREEN_PARAM)

    QT_SETTINGS_PROPERTY(bool, keyAfterLoginSendEnable, false, MPParams::KEY_AFTER_LOGIN_SEND_BOOL_PARAM)
    QT_SETTINGS_PROPERTY(int, keyAfterLoginSend, 0, MPParams::KEY_AFTER_LOGIN_SEND_PARAM)
    QT_SETTINGS_PROPERTY(bool, keyAfterPassSendEnable, false, MPParams::KEY_AFTER_PASS_SEND_BOOL_PARAM)
    QT_SETTINGS_PROPERTY(int, keyAfterPassSend, 0, MPParams::KEY_AFTER_PASS_SEND_PARAM)
    QT_SETTINGS_PROPERTY(bool, delayAfterKeyEntryEnable, false, MPParams::DELAY_AFTER_KEY_ENTRY_BOOL_PARAM)
    QT_SETTINGS_PROPERTY(int, delayAfterKeyEntry, 0, MPParams::DELAY_AFTER_KEY_ENTRY_PARAM)



    //MP Mini only
    QT_SETTINGS_PROPERTY(int, screenBrightness, 0, MPParams::MINI_OLED_CONTRAST_CURRENT_PARAM) //51-20%, 89-35%, 128-50%, 166-65%, 204-80%, 255-100%
    QT_SETTINGS_PROPERTY(bool, knockEnabled, false, MPParams::MINI_KNOCK_DETECT_ENABLE_PARAM)
    QT_SETTINGS_PROPERTY(int, knockSensitivity, 0, MPParams::MINI_KNOCK_THRES_PARAM) // 0-very low, 1-low, 2-medium, 3-high
    QT_SETTINGS_PROPERTY(bool, randomStartingPin, false, MPParams::RANDOM_INIT_PIN_PARAM)
    QT_SETTINGS_PROPERTY(bool, hashDisplay, false, MPParams::HASH_DISPLAY_FEATURE_PARAM)
    QT_SETTINGS_PROPERTY(int, lockUnlockMode, 0, MPParams::LOCK_UNLOCK_FEATURE_PARAM)

public:
    MPSettings(MPDevice *parent, IMessageProtocol *mesProt);
    virtual ~MPSettings();

    enum KnockSensitivityThreshold
    {
        KNOCKING_VERY_LOW = 15,
        KNOCKING_LOW = 11,
        KNOCKING_MEDIUM = 8,
        KNOCKING_HIGH = 5
    };

    //reload parameters from MP
    void loadParameters();
    MPParams::Param getParamId(const QString& paramName);
    QString getParamName(MPParams::Param param);
    void sendEveryParameter();

    void updateParam(MPParams::Param param, bool en);
    void updateParam(MPParams::Param param, int val);

private:
    void fillParameterMapping();


    MPDevice* mpDevice = nullptr;
    IMessageProtocol* pMesProt = nullptr;
    QMap<MPParams::Param, QString> m_paramMap;

    //flag set when loading all parameters
    bool m_readingParams = false;
};

#endif // MPSETTINGS_H
