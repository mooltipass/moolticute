#include "MPSettings.h"
#include "AsyncJobs.h"
#include "MPDevice.h"
#include "IMessageProtocol.h"
#include "WSServerCon.h"

MPSettings::MPSettings(MPDevice *parent, IMessageProtocol *mesProt)
    : QObject(parent),
      mpDevice(parent),
      pMesProt(mesProt)
{
    fillParameterMapping();
}

MPSettings::~MPSettings()
{

}

void MPSettings::loadParameters()
{
    m_readingParams = true;
    AsyncJobs *jobs = new AsyncJobs(
                          "Loading device parameters",
                          this);

    if (mpDevice->isBLE())
    {
        //TODO: Implement settings for BLE
        jobs->append(new MPCommandJob(mpDevice,
                       MPCmd::GET_DEVICE_SETTINGS,
                       [this] (const QByteArray &data, bool &)
                        {
                            qDebug() << "Full device settings payload: " << pMesProt->getFullPayload(data).toHex();
                            qWarning() << "Load Parameters processing haven't been implemented for BLE yet.";
                            return true;
                        }
        ));
        mpDevice->enqueueAndRunJob(jobs);
        return;
    }

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::VERSION,
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto flashSize = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received MP version FLASH size: " << flashSize << "Mb";
        QString hw = QString(pMesProt->getPayloadBytes(data, 1, pMesProt->getMessageSize(data) - 2));
        qDebug() << "received MP version hw: " << hw;
        set_flashMbSize(flashSize);
        mpDevice->set_hwVersion(hw);

        QRegularExpressionMatchIterator i = regVersion.globalMatch(hw);
        if (i.hasNext())
        {
            QRegularExpressionMatch match = i.next();
            int v = match.captured(1).toInt() * 10 +
                    match.captured(2).toInt();
            mpDevice->isFw12Flag = v >= 12;
            if (match.captured(3) == "_mini")
            {
                mpDevice->deviceType = DeviceType::MINI;
            }
        }

        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::KEYBOARD_LAYOUT_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto language = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received language: " << language;
        set_keyboardLayout(language);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::LOCK_TIMEOUT_ENABLE_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto lockTimeoutEnabled = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received lock timeout enable: " << lockTimeoutEnabled;
        set_lockTimeoutEnabled(lockTimeoutEnabled != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::LOCK_TIMEOUT_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto lockTimeout = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received lock timeout: " << lockTimeout;
        set_lockTimeout(lockTimeout);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::SCREENSAVER_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto screensaver = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received screensaver: " << screensaver;
        set_screensaver(screensaver != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::USER_REQ_CANCEL_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto userRequestCancel = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received userRequestCancel: " << userRequestCancel;
        set_userRequestCancel(userRequestCancel != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::USER_INTER_TIMEOUT_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto userInteractionTimeout = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received userInteractionTimeout: " << userInteractionTimeout;
        set_userInteractionTimeout(userInteractionTimeout);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::FLASH_SCREEN_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto flashScreen = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received flashScreen: " << flashScreen;
        set_flashScreen(flashScreen != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::OFFLINE_MODE_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto offlineMode = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received offlineMode: " << offlineMode;
        set_offlineMode(offlineMode != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::TUTORIAL_BOOL_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto tutorialEnabled = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received tutorialEnabled: " << tutorialEnabled;
        set_tutorialEnabled(tutorialEnabled != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::MINI_OLED_CONTRAST_CURRENT_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto screenBrightness = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received screenBrightness: " << screenBrightness;
        set_screenBrightness(screenBrightness);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::MINI_KNOCK_DETECT_ENABLE_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto knockEnabled = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received set_knockEnabled: " << knockEnabled;
        set_knockEnabled(knockEnabled != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::MINI_KNOCK_THRES_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto knockEnabled = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received knock threshold: " << knockEnabled;

        // The conversion of the real-device 'knock threshold' property
        // to our made-up 'knock sensitivity' property:
        int v;
        quint8 s = knockEnabled;
        if      (s >= KNOCKING_VERY_LOW) v = 0;
        else if (s >= KNOCKING_LOW)      v = 1;
        else if (s >= KNOCKING_MEDIUM)   v = 2;
        else v = 3;
        set_knockSensitivity(v);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::RANDOM_INIT_PIN_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto randomStartingPin = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received randomStartingPin: " << randomStartingPin;
        set_randomStartingPin(randomStartingPin != 0);
        return true;
    }));


    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::HASH_DISPLAY_FEATURE_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto hashDisplay = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received hashDisplay: " << hashDisplay;
        set_hashDisplay(hashDisplay != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::LOCK_UNLOCK_FEATURE_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto lockUnlockMode = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received lockUnlockMode: " << lockUnlockMode;
        set_lockUnlockMode(lockUnlockMode);
        return true;
    }));


    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::KEY_AFTER_LOGIN_SEND_BOOL_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto keyAfterLoginSend = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received key after login send enabled: " << keyAfterLoginSend;
        set_keyAfterLoginSendEnable(keyAfterLoginSend != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::KEY_AFTER_LOGIN_SEND_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto keyAfterLoginSend = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received key after login send " << keyAfterLoginSend;
        set_keyAfterLoginSend(keyAfterLoginSend);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::KEY_AFTER_PASS_SEND_BOOL_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto keyAfterPassSend = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received key after pass send enabled: " << keyAfterPassSend;
        set_keyAfterPassSendEnable(keyAfterPassSend != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::KEY_AFTER_PASS_SEND_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto keyAfterPassSend = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received key after pass send " << keyAfterPassSend;
        set_keyAfterPassSend(keyAfterPassSend);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::DELAY_AFTER_KEY_ENTRY_BOOL_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto delayAfterKeyEntry = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received delay after key entry enabled: " << delayAfterKeyEntry;
        set_delayAfterKeyEntryEnable(delayAfterKeyEntry != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::DELAY_AFTER_KEY_ENTRY_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto delayAfterKeyEntry = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received delay after key entry " << delayAfterKeyEntry;
        set_delayAfterKeyEntry(delayAfterKeyEntry);
        return true;
    }));

    connect(jobs, &AsyncJobs::finished, [this](const QByteArray &data)
    {
        Q_UNUSED(data);
        //data is last result
        //all jobs finished success
        qInfo() << "Finished loading device options";

        if (mpDevice->isFw12() && mpDevice->isMini())
        {
            qInfo() << "Mini firmware above v1.2, requesting serial number";

            AsyncJobs* v12jobs = new AsyncJobs("Loading device serial number", this);

            /* Query serial number */
            v12jobs->append(new MPCommandJob(mpDevice,
                                          MPCmd::GET_SERIAL,
                                          [this](const QByteArray &data, bool &) -> bool
            {
                const auto serialNumber = pMesProt->getSerialNumber(data);
                mpDevice->set_serialNumber(serialNumber);
                qDebug() << "Mooltipass Mini serial number:" << serialNumber;
                return true;
            }));

            connect(v12jobs, &AsyncJobs::finished, [this](const QByteArray &data)
            {
                Q_UNUSED(data);
                //data is last result
                //all jobs finished success
                m_readingParams = false;
                qInfo() << "Finished loading Mini serial number";
            });

            connect(v12jobs, &AsyncJobs::failed, [this](AsyncJob *failedJob)
            {
                Q_UNUSED(failedJob);
                qCritical() << "Loading Mini serial number failed";
                m_readingParams = false;
                loadParameters(); // memory: does it get "piled on?"
            });
            mpDevice->enqueueAndRunJob(v12jobs);
        }
    });

    connect(jobs, &AsyncJobs::failed, [this](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Loading option failed";
        m_readingParams = false;
        loadParameters(); // memory: does it get "piled on?"
    });

    mpDevice->enqueueAndRunJob(jobs);
}

MPParams::Param MPSettings::getParamId(const QString &paramName)
{
    return m_paramMap.key(paramName);
}

QString MPSettings::getParamName(MPParams::Param param)
{
    return m_paramMap[param];
}

void MPSettings::sendEveryParameter()
{
    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); ++i)
    {
        const auto signalName = metaObject()->property(i).notifySignal().name();
        QVariant value = metaObject()->property(i).read(this);
        if (value.type() == QVariant::Bool)
        {
            emit QMetaObject::invokeMethod(this, signalName, Q_ARG(bool, value.toBool()));
        }
        else
        {
            emit QMetaObject::invokeMethod(this, signalName, Q_ARG(int, value.toInt()));
        }
    }
}

void MPSettings::connectSendParams(WSServerCon *wsServerCon)
{
    //Connect every propertyChanged signals to the correct sendParams slot
    for (int i = metaObject()->propertyOffset(); i < metaObject()->propertyCount(); ++i)
    {
        const auto signal = metaObject()->property(i).notifySignal();
        const bool isFirstParamBool = signal.parameterType(0) == QVariant::Bool;
        const auto slot = wsServerCon->metaObject()->method(
                            wsServerCon->metaObject()->indexOfMethod(
                                isFirstParamBool ? "sendParams(bool,int)" : "sendParams(int,int)"
                          ));
        connect(this, signal, wsServerCon, slot);
    }
}

void MPSettings::updateParam(MPParams::Param param, int val)
{
    if (MPParams::MINI_KNOCK_THRES_PARAM == param)
    {
        convertKnockValue(val);
    }
    else if (MPParams::USER_INTER_TIMEOUT_PARAM == param || MPParams::LOCK_TIMEOUT_PARAM == param)
    {
        checkTimeoutBoundaries(val);
    }

    QMetaEnum m = QMetaEnum::fromType<MPParams::Param>();
    QString logInf = QStringLiteral("Updating %1 param: %2").arg(m.valueToKey(param)).arg(val);

    AsyncJobs *jobs = new AsyncJobs(logInf, this);

    QByteArray ba;
    ba.append(static_cast<quint8>(param));
    ba.append(static_cast<quint8>(val));

    jobs->append(new MPCommandJob(mpDevice, MPCmd::SET_MOOLTIPASS_PARM, ba, pMesProt->getDefaultFuncDone()));

    connect(jobs, &AsyncJobs::finished, [param](const QByteArray &)
    {
        qInfo() << param << " param updated with success";
    });
    connect(jobs, &AsyncJobs::failed, [param](AsyncJob *)
    {
        qWarning() << "Failed to change " << param;
    });

    mpDevice->enqueueAndRunJob(jobs);
}

void MPSettings::fillParameterMapping()
{
    m_paramMap = {
        {MPParams::KEYBOARD_LAYOUT_PARAM, "keyboard_layout"},
        {MPParams::LOCK_TIMEOUT_ENABLE_PARAM, "lock_timeout_enabled"},
        {MPParams::LOCK_TIMEOUT_PARAM, "lock_timeout"},
        {MPParams::SCREENSAVER_PARAM, "screensaver"},
        {MPParams::USER_REQ_CANCEL_PARAM, "user_request_cancel"},
        {MPParams::USER_INTER_TIMEOUT_PARAM, "user_interaction_timeout"},
        {MPParams::FLASH_SCREEN_PARAM, "flash_screen"},
        {MPParams::OFFLINE_MODE_PARAM, "offline_mode"},
        {MPParams::TUTORIAL_BOOL_PARAM, "tutorial_enabled"},
        {MPParams::MINI_OLED_CONTRAST_CURRENT_PARAM, "screen_brightness"},
        {MPParams::MINI_KNOCK_DETECT_ENABLE_PARAM, "knock_enabled"},
        {MPParams::MINI_KNOCK_THRES_PARAM, "knock_sensitivity"},
        {MPParams::RANDOM_INIT_PIN_PARAM, "random_starting_pin"},
        {MPParams::HASH_DISPLAY_FEATURE_PARAM, "hash_display"},
        {MPParams::LOCK_UNLOCK_FEATURE_PARAM, "lock_unlock_mode"},
        {MPParams::KEY_AFTER_LOGIN_SEND_BOOL_PARAM, "key_after_login_enabled"},
        {MPParams::KEY_AFTER_LOGIN_SEND_PARAM, "key_after_login"},
        {MPParams::KEY_AFTER_PASS_SEND_BOOL_PARAM, "key_after_pass_enabled"},
        {MPParams::KEY_AFTER_PASS_SEND_PARAM, "key_after_pass"},
        {MPParams::DELAY_AFTER_KEY_ENTRY_BOOL_PARAM, "delay_after_key_enabled"},
        {MPParams::DELAY_AFTER_KEY_ENTRY_PARAM, "delay_after_key"}
    };
}

void MPSettings::convertKnockValue(int &val)
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

void MPSettings::checkTimeoutBoundaries(int &val)
{
    if (val < 0) val = 0;
    if (val > 0xFF) val = 0xFF;
}

void MPSettings::updateParam(MPParams::Param param, bool en)
{
    updateParam(param, static_cast<int>(en));
}
