#include "MPSettingsBLE.h"
#include "AsyncJobs.h"
#include "MPDevice.h"
#include "IMessageProtocol.h"
#include "WSServerCon.h"

MPSettingsBLE::MPSettingsBLE(MPDevice *parent, IMessageProtocol *mesProt)
    : MPSettings(parent, mesProt)
{
    fillParameterMapping();
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
                        m_settings = pMesProt->getFullPayload(data);
                        qDebug() << "Full device settings payload: " << m_settings.toHex();
                        qWarning() << "Load Parameters processing haven't been implemented for BLE yet.";
                        set_randomStartingPin(m_settings.at(RANDOM_PIN_BYTE_ID) != 0);
                        return true;
                    }
    ));
    mpDevice->enqueueAndRunJob(jobs);
    return;
}

void MPSettingsBLE::updateParam(MPParams::Param param, int val)
{
    //TODO: Updating params, when implemented on fw side
    if (param == MPParams::RANDOM_INIT_PIN_PARAM)
    {
        m_settings[RANDOM_PIN_BYTE_ID] = static_cast<char>(val);
    }
}

void MPSettingsBLE::connectSendParams(WSServerCon *wsServerCon)
{
    MPSettings::connectSendParams(wsServerCon);
    connect(wsServerCon, SIGNAL(parameterProcessFinished()), this, SLOT(setSettings()));
}

void MPSettingsBLE::setSettings()
{
    AsyncJobs *jobs = new AsyncJobs(
                          "Setting device parameters",
                          this);

    jobs->append(new MPCommandJob(mpDevice,
                   MPCmd::SET_DEVICE_SETTINGS,
                   m_settings,
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


void MPSettingsBLE::fillParameterMapping()
{
    m_paramMap.insert(MPParams::PROMPT_ANIMATION_PARAM, "prompt_animation");
}
