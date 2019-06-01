#include "SettingGuiMini.h"

SettingGuiMini::SettingGuiMini(QObject* parent)
    : DeviceSettingsMini(parent)
{

}

void SettingGuiMini::updateParam(MPParams::Param param, int val)
{
    setProperty(getParamName(param), val);
}

void SettingGuiMini::loadParameters()
{
    qCritical() << "Not implemented yet";
}
