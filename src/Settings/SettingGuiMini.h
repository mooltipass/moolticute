#ifndef SETTINGGUIMINI_H
#define SETTINGGUIMINI_H

#include <QObject>
#include "DeviceSettingsMini.h"

class SettingGuiMini : public DeviceSettingsMini
{
    Q_OBJECT

public:
    SettingGuiMini(QObject* parent);
    void loadParameters() override;
    void updateParam(MPParams::Param param, int val) override;
};

#endif // SETTINGGUIMINI_H
