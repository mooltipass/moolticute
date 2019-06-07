#include "MPSettingsBLE.h"
#include "AsyncJobs.h"
#include "MPDevice.h"
#include "IMessageProtocol.h"
#include "WSServerCon.h"

MPSettingsBLE::MPSettingsBLE(MPDevice *parent, IMessageProtocol *mesProt)
    : DeviceSettingsBLE(parent),
      mpDevice{parent},
      pMesProt{mesProt}
{
}

void MPSettingsBLE::loadParameters()
{
    m_readingParams = true;
    AsyncJobs *jobs = new AsyncJobs(
                          "Loading device parameters",
                          this);

    //TODO: Implement loading correct settings, when implemented on fw side
    jobs->append(new MPCommandJob(mpDevice,
                   MPCmd::GET_DEVICE_SETTINGS,
                   [this] (const QByteArray &data, bool &)
                    {
                        m_lastDeviceSettings = pMesProt->getFullPayload(data);
                        qDebug() << "Full device settings payload: " << m_lastDeviceSettings.toHex();
                        qWarning() << "Load Parameters processing haven't been implemented for BLE yet.";
                        set_reserved_ble(m_lastDeviceSettings.at(DeviceSettingsBLE::RESERVED_BYTE) != 0);
                        set_user_interaction_timeout(m_lastDeviceSettings.at(DeviceSettingsBLE::USER_INTERACTION_TIMEOUT_BYTE));
                        set_random_starting_pin(m_lastDeviceSettings.at(DeviceSettingsBLE::RANDOM_PIN_BYTE) != 0);
                        set_prompt_animation(m_lastDeviceSettings.at(DeviceSettingsBLE::ANIMATION_DURING_PROMPT_BYTE) != 0);
                        return true;
                    }
    ));
    mpDevice->enqueueAndRunJob(jobs);
    return;
}

void MPSettingsBLE::updateParam(MPParams::Param param, int val)
{
    if (m_bleByteMapping.contains(param))
    {
        m_lastDeviceSettings[m_bleByteMapping[param]] = static_cast<char>(val);
    }
}

void MPSettingsBLE::connectSendParams(QObject *slotObject)
{
    DeviceSettings::connectSendParams(slotObject);
    connect(slotObject, SIGNAL(parameterProcessFinished()), this, SLOT(setSettings()));
}

void MPSettingsBLE::setSettings()
{
    AsyncJobs *jobs = new AsyncJobs(
                          "Setting device parameters",
                          this);

    jobs->append(new MPCommandJob(mpDevice,
                   MPCmd::SET_DEVICE_SETTINGS,
                   m_lastDeviceSettings,
                   [this] (const QByteArray &data, bool &)
                    {
                        if (0x01 == pMesProt->getFirstPayloadByte(data))
                        {
                            qDebug() << "Set device settings was successfull";
                        }
                        else
                        {
                            qWarning() << "Set device settings failed";
                        }
                        return true;
                    }
    ));
    mpDevice->enqueueAndRunJob(jobs);
}


