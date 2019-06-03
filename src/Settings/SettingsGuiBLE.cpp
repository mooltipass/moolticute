#include "SettingsGuiBLE.h"
#include "MainWindow.h"
#include "ui_MainWindow.h"

SettingsGuiBLE::SettingsGuiBLE(QObject *parent)
    : DeviceSettingsBLE(parent)
{

}

void SettingsGuiBLE::loadParameters()
{
    qCritical() << "Not implemented yet";
}

void SettingsGuiBLE::updateParam(MPParams::Param param, int val)
{
    setProperty(getParamName(param), val);
}

void SettingsGuiBLE::createSettingUIMapping(MainWindow *mw)
{
    m_mw = mw;
}

bool SettingsGuiBLE::checkSettingsChanged()
{
    //TODO implement ui part
    return false;
}

void SettingsGuiBLE::getChangedSettings(QJsonObject &o)
{
    Q_UNUSED(o);
    //TODO implement ui part
}
