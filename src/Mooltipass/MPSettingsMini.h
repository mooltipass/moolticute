#ifndef MPSETTINGSMINI_H
#define MPSETTINGSMINI_H

#include "DeviceSettingsMini.h"

class MPDevice;
class IMessageProtocol;

class MPSettingsMini : public DeviceSettingsMini
{
    Q_OBJECT

public:
    MPSettingsMini(MPDevice *parent, IMessageProtocol *mesProt);
    virtual ~MPSettingsMini() override = default;

    void loadParameters() override;
    void updateParam(MPParams::Param param, int val) override;

private:
    MPDevice* mpDevice = nullptr;
    IMessageProtocol* pMesProt = nullptr;
};

#endif // MPSETTINGSMINI_H
