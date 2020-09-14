#ifndef DEVICESETTINGS_H
#define DEVICESETTINGS_H

#include <QObject>
#include "QtHelper.h"
#include "Common.h"
#include "MooltipassCmds.h"

class DeviceSettings : public QObject
{
    Q_OBJECT

    QT_SETTINGS_PROPERTY(int, key_after_login, 0, MPParams::KEY_AFTER_LOGIN_SEND_PARAM)
    QT_SETTINGS_PROPERTY(int, key_after_pass, 0, MPParams::KEY_AFTER_PASS_SEND_PARAM)
    QT_SETTINGS_PROPERTY(int, delay_after_key, 0, MPParams::DELAY_AFTER_KEY_ENTRY_PARAM)
    QT_SETTINGS_PROPERTY(bool, tutorial_enabled, false, MPParams::TUTORIAL_BOOL_PARAM)

    QT_SETTINGS_PROPERTY(int, user_interaction_timeout, 0, MPParams::USER_INTER_TIMEOUT_PARAM)
    QT_SETTINGS_PROPERTY(bool, random_starting_pin, false, MPParams::RANDOM_INIT_PIN_PARAM)
    QT_SETTINGS_PROPERTY(int, knock_sensitivity, 0, MPParams::MINI_KNOCK_THRES_PARAM) // 0-very low, 1-low, 2-medium, 3-high
    QT_SETTINGS_PROPERTY(int, lock_unlock_mode, 0, MPParams::LOCK_UNLOCK_FEATURE_PARAM)

public:
    DeviceSettings(QObject *parent);
    virtual ~DeviceSettings(){}

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
    virtual void connectSendParams(QObject* slotObject);
    void setProperty(const QString& propName, int value);

    //reload parameters from MP
    virtual void loadParameters() = 0;
    virtual void updateParam(MPParams::Param param, int val) = 0;
    void updateParam(MPParams::Param param, bool en);
    virtual void setupKeyboardLayout(bool) {}

    const QMetaObject* getMetaObject() const {return metaObject();}

protected:
    void sendEveryParameter(const QMetaObject* meta);
    void connectSendParams(QObject* slotObject, const QMetaObject* meta);
    bool setProperty(const QString& propName, int value, const QMetaObject* meta);

    void fillParameterMapping();

    void convertKnockValue(int& val);
    int convertBackKnockValue(int val);

    QMap<MPParams::Param, QString> m_paramMap;

    //flag set when loading all parameters
    bool m_readingParams = false;
};

#endif // DEVICESETTINGS_H
