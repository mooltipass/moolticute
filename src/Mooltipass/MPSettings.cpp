#include "MPSettings.h"
#include "AsyncJobs.h"
#include "MPDevice.h"
#include "IMessageProtocol.h"

MPSettings::MPSettings(MPDevice *parent, IMessageProtocol *mesProt)
    : QObject(parent),
      mpDevice(parent),
      pMesProt(mesProt)
{

}

MPSettings::~MPSettings()
{

}

void MPSettings::loadParameters()
{
    readingParams = true;
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
        mpDevice->jobsQueue.enqueue(jobs);
        mpDevice->runAndDequeueJobs();
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
        set_hwVersion(hw);

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
                set_serialNumber(serialNumber);
                qDebug() << "Mooltipass Mini serial number:" << serialNumber;
                return true;
            }));

            connect(v12jobs, &AsyncJobs::finished, [this](const QByteArray &data)
            {
                Q_UNUSED(data);
                //data is last result
                //all jobs finished success
                readingParams = false;
                qInfo() << "Finished loading Mini serial number";
            });

            connect(v12jobs, &AsyncJobs::failed, [this](AsyncJob *failedJob)
            {
                Q_UNUSED(failedJob);
                qCritical() << "Loading Mini serial number failed";
                readingParams = false;
                loadParameters(); // memory: does it get "piled on?"
            });
            mpDevice->jobsQueue.enqueue(v12jobs);
            mpDevice->runAndDequeueJobs();
        }
    });

    connect(jobs, &AsyncJobs::failed, [this](AsyncJob *failedJob)
    {
        Q_UNUSED(failedJob);
        qCritical() << "Loading option failed";
        readingParams = false;
        loadParameters(); // memory: does it get "piled on?"
    });

    mpDevice->jobsQueue.enqueue(jobs);
    mpDevice->runAndDequeueJobs();
}

void MPSettings::updateKeyboardLayout(int lang)
{
    updateParam(MPParams::KEYBOARD_LAYOUT_PARAM, lang);
}

void MPSettings::updateLockTimeoutEnabled(bool en)
{
    updateParam(MPParams::LOCK_TIMEOUT_ENABLE_PARAM, en);
}

void MPSettings::updateLockTimeout(int timeout)
{
    if (timeout < 0) timeout = 0;
    if (timeout > 0xFF) timeout = 0xFF;
    updateParam(MPParams::LOCK_TIMEOUT_PARAM, timeout);
}

void MPSettings::updateScreensaver(bool en)
{
    updateParam(MPParams::SCREENSAVER_PARAM, en);
}

void MPSettings::updateUserRequestCancel(bool en)
{
    updateParam(MPParams::USER_REQ_CANCEL_PARAM, en);
}

void MPSettings::updateUserInteractionTimeout(int timeout)
{
    if (timeout < 0) timeout = 0;
    if (timeout > 0xFF) timeout = 0xFF;
    updateParam(MPParams::USER_INTER_TIMEOUT_PARAM, timeout);
}

void MPSettings::updateFlashScreen(bool en)
{
    updateParam(MPParams::FLASH_SCREEN_PARAM, en);
}

void MPSettings::updateOfflineMode(bool en)
{
    updateParam(MPParams::OFFLINE_MODE_PARAM, en);
}

void MPSettings::updateTutorialEnabled(bool en)
{
    updateParam(MPParams::TUTORIAL_BOOL_PARAM, en);
}

void MPSettings::updateScreenBrightness(int bval) //In percent
{
    updateParam(MPParams::MINI_OLED_CONTRAST_CURRENT_PARAM, bval);
}

void MPSettings::updateKnockEnabled(bool en)
{
    updateParam(MPParams::MINI_KNOCK_DETECT_ENABLE_PARAM, en);
}

void MPSettings::updateKeyAfterLoginSendEnable(bool en)
{
    updateParam(MPParams::KEY_AFTER_LOGIN_SEND_BOOL_PARAM, en);
}

void MPSettings::updateKeyAfterLoginSend(int value)
{
    updateParam(MPParams::KEY_AFTER_LOGIN_SEND_PARAM, value);
}

void MPSettings::updateKeyAfterPassSendEnable(bool en)
{
     updateParam(MPParams::KEY_AFTER_PASS_SEND_BOOL_PARAM, en);
}

void MPSettings::updateKeyAfterPassSend(int value)
{
    updateParam(MPParams::KEY_AFTER_PASS_SEND_PARAM, value);
}

void MPSettings::updateDelayAfterKeyEntryEnable(bool en)
{
    updateParam(MPParams::DELAY_AFTER_KEY_ENTRY_BOOL_PARAM, en);
}

void MPSettings::updateDelayAfterKeyEntry(int val)
{
    updateParam(MPParams::DELAY_AFTER_KEY_ENTRY_PARAM, val);
}

void MPSettings::updateKnockSensitivity(int s) // 0-very low, 1-low, 2-medium, 3-high
{
    quint8 v;
    switch(s)
    {
    case 0: v = KNOCKING_VERY_LOW; break;
    case 1: v = KNOCKING_LOW; break;
    case 2: v = KNOCKING_MEDIUM; break;
    case 3: v = KNOCKING_HIGH; break;
    default:
        v = KNOCKING_MEDIUM;
    }

    updateParam(MPParams::MINI_KNOCK_THRES_PARAM, v);
}

void MPSettings::updateRandomStartingPin(bool en)
{
    updateParam(MPParams::RANDOM_INIT_PIN_PARAM, en);
}

void MPSettings::updateHashDisplay(bool en)
{
    updateParam(MPParams::HASH_DISPLAY_FEATURE_PARAM, en);
}

void MPSettings::updateLockUnlockMode(int val)
{
    updateParam(MPParams::LOCK_UNLOCK_FEATURE_PARAM, val);
}

void MPSettings::updateParam(MPParams::Param param, int val)
{
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

    mpDevice->jobsQueue.enqueue(jobs);
    mpDevice->runAndDequeueJobs();
}

void MPSettings::updateParam(MPParams::Param param, bool en)
{
    updateParam(param, (int)en);
}
