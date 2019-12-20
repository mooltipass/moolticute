#include "MPSettingsMini.h"
#include "AsyncJobs.h"
#include "MPDevice.h"
#include "IMessageProtocol.h"

MPSettingsMini::MPSettingsMini(MPDevice *parent, IMessageProtocol *mesProt)
    : DeviceSettingsMini(parent),
      mpDevice{parent},
      pMesProt{mesProt}
{
}

void MPSettingsMini::loadParameters()
{
    m_readingParams = true;
    AsyncJobs *jobs = new AsyncJobs(
                          "Loading device parameters",
                          this);

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::VERSION,
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto flashSize = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received MP version FLASH size: " << flashSize << "Mb";
        QString hw = QString(pMesProt->getPayloadBytes(data, 1, pMesProt->getMessageSize(data) - 2));
        qDebug() << "received MP version hw: " << hw;
        mpDevice->set_flashMbSize(flashSize);
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
        set_keyboard_layout(language);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::LOCK_TIMEOUT_ENABLE_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto lockTimeoutEnabled = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received lock timeout enable: " << lockTimeoutEnabled;
        set_lock_timeout_enabled(lockTimeoutEnabled != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::LOCK_TIMEOUT_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto lockTimeout = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received lock timeout: " << lockTimeout;
        set_lock_timeout(lockTimeout);
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
        set_user_request_cancel(userRequestCancel != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::USER_INTER_TIMEOUT_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto userInteractionTimeout = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received userInteractionTimeout: " << userInteractionTimeout;
        set_user_interaction_timeout(userInteractionTimeout);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::FLASH_SCREEN_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto flashScreen = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received flashScreen: " << flashScreen;
        set_flash_screen(flashScreen != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::OFFLINE_MODE_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto offlineMode = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received offlineMode: " << offlineMode;
        set_offline_mode(offlineMode != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::TUTORIAL_BOOL_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto tutorialEnabled = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received tutorialEnabled: " << tutorialEnabled;
        set_tutorial_enabled(tutorialEnabled != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::MINI_OLED_CONTRAST_CURRENT_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto screenBrightness = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received screenBrightness: " << screenBrightness;
        set_screen_brightness(screenBrightness);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::MINI_KNOCK_DETECT_ENABLE_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto knockEnabled = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received set_knockEnabled: " << knockEnabled;
        set_knock_enabled(knockEnabled != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::MINI_KNOCK_THRES_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto knockSensitivity = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received knock threshold: " << knockSensitivity;

        // The conversion of the real-device 'knock threshold' property
        // to our made-up 'knock sensitivity' property:
        set_knock_sensitivity(convertBackKnockValue(knockSensitivity));
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::RANDOM_INIT_PIN_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto randomStartingPin = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received randomStartingPin: " << randomStartingPin;
        set_random_starting_pin(randomStartingPin != 0);
        return true;
    }));


    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::HASH_DISPLAY_FEATURE_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto hashDisplay = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received hashDisplay: " << hashDisplay;
        set_hash_display(hashDisplay != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::LOCK_UNLOCK_FEATURE_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto lockUnlockMode = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received lockUnlockMode: " << lockUnlockMode;
        set_lock_unlock_mode(lockUnlockMode);
        return true;
    }));


    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::KEY_AFTER_LOGIN_SEND_BOOL_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto keyAfterLoginSend = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received key after login send enabled: " << keyAfterLoginSend;
        set_key_after_login_enabled(keyAfterLoginSend != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::KEY_AFTER_LOGIN_SEND_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto keyAfterLoginSend = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received key after login send " << keyAfterLoginSend;
        set_key_after_login(keyAfterLoginSend);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::KEY_AFTER_PASS_SEND_BOOL_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto keyAfterPassSend = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received key after pass send enabled: " << keyAfterPassSend;
        set_key_after_pass_enabled(keyAfterPassSend != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::KEY_AFTER_PASS_SEND_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto keyAfterPassSend = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received key after pass send " << keyAfterPassSend;
        set_key_after_pass(keyAfterPassSend);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::DELAY_AFTER_KEY_ENTRY_BOOL_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto delayAfterKeyEntry = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received delay after key entry enabled: " << delayAfterKeyEntry;
        set_delay_after_key_enabled(delayAfterKeyEntry != 0);
        return true;
    }));

    jobs->append(new MPCommandJob(mpDevice,
                                  MPCmd::GET_MOOLTIPASS_PARM,
                                  QByteArray(1, MPParams::DELAY_AFTER_KEY_ENTRY_PARAM),
                                  [this](const QByteArray &data, bool &) -> bool
    {
        const auto delayAfterKeyEntry = pMesProt->getFirstPayloadByte(data);
        qDebug() << "received delay after key entry " << delayAfterKeyEntry;
        set_delay_after_key(delayAfterKeyEntry);
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

void MPSettingsMini::updateParam(MPParams::Param param, int val)
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
