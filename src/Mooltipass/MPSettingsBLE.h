#ifndef MPSETTINGSBLE_H
#define MPSETTINGSBLE_H

#include "MPSettings.h"

class MPSettingsBLE : public MPSettings
{
    Q_OBJECT

    //MP BLE only
    QT_SETTINGS_PROPERTY(bool, promptAnimation, false, MPParams::PROMPT_ANIMATION_PARAM)

public:
    MPSettingsBLE(MPDevice *parent, IMessageProtocol *mesProt);

    void loadParameters() override;
    void updateParam(MPParams::Param param, int val) override;

private:
    void fillParameterMapping();
};

#endif // MPSETTINGSBLE_H
