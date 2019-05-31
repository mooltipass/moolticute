#include "MPSettings.h"
#include "AsyncJobs.h"
#include "MPDevice.h"
#include "IMessageProtocol.h"
#include "WSServerCon.h"

MPSettings::MPSettings(MPDevice *parent, IMessageProtocol *mesProt)
    : mpDevice(parent),
      pMesProt(mesProt)
{
}

MPSettings::~MPSettings()
{
}

void MPSettings::updateParam(MPParams::Param param, bool en)
{
    updateParam(param, static_cast<int>(en));
}
