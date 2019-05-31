#ifndef MPSETTINGSMINI_H
#define MPSETTINGSMINI_H

#include "MPSettings.h"
#include "DeviceSettingsMini.h"

class MPSettingsMini : public DeviceSettingsMini, public MPSettings
{
    Q_OBJECT

public:
    MPSettingsMini(MPDevice *parent, IMessageProtocol *mesProt);
    virtual ~MPSettingsMini() {}

    void loadParameters() override;
    void updateParam(MPParams::Param param, int val) override;
};

#endif // MPSETTINGSMINI_H
