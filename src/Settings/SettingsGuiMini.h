#ifndef SETTINGSGUIMINI_H
#define SETTINGSGUIMINI_H

#include <QObject>
#include "ISettingsGui.h"
#include "DeviceSettingsMini.h"

class MainWindow;

class SettingsGuiMini : public DeviceSettingsMini, public ISettingsGui
{
    Q_OBJECT

public:
    explicit SettingsGuiMini(QObject* parent, MainWindow* mw);
    void loadParameters() override;
    void updateParam(MPParams::Param param, int val) override;
    void createSettingUIMapping() override;
};

#endif // SETTINGSGUIMINI_H
