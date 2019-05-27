#include "MPSettingsBLE.h"
#include "AsyncJobs.h"
#include "MPDevice.h"
#include "IMessageProtocol.h"

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

void MPSettingsBLE::updateParam(MPParams::Param param, int val)
{
    Q_UNUSED(param)
    Q_UNUSED(val)
    //TODO implement
}

void MPSettingsBLE::fillParameterMapping()
{
    m_paramMap.insert(MPParams::PROMPT_ANIMATION_PARAM, "prompt_animation");
}
