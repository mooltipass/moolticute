#ifndef MPSETTINGS_H
#define MPSETTINGS_H

#include <QObject>
#include "QtHelper.h"
#include "Common.h"
#include "MooltipassCmds.h"

class MPDevice;
class IMessageProtocol;
class WSServerCon;

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

    QT_SETTINGS_PROPERTY(bool, randomStartingPin, false, MPParams::RANDOM_INIT_PIN_PARAM)

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

    MPParams::Param getParamId(const QString& paramName);
    QString getParamName(MPParams::Param param);
    void sendEveryParameter();
    virtual void connectSendParams(WSServerCon* wsServerCon);

    //reload parameters from MP
    virtual void loadParameters() = 0;
    virtual void updateParam(MPParams::Param param, int val) = 0;
    void updateParam(MPParams::Param param, bool en);

protected:
    void fillParameterMapping();

    void sendEveryParameter(const QMetaObject* meta);
    void connectSendParams(WSServerCon* wsServerCon, const QMetaObject* meta);

    MPDevice* mpDevice = nullptr;
    IMessageProtocol* pMesProt = nullptr;
    QMap<MPParams::Param, QString> m_paramMap;

    //flag set when loading all parameters
    bool m_readingParams = false;
};

#endif // MPSETTINGS_H
