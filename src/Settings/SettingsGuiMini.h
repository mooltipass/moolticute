#ifndef SETTINGSGUIMINI_H
#define SETTINGSGUIMINI_H

#include <QObject>
#include "DeviceSettingsMini.h"

class MainWindow;

class SettingsGuiMini : public DeviceSettingsMini
{
    Q_OBJECT

public:
    SettingsGuiMini(QObject* parent);
    void loadParameters() override;
    void updateParam(MPParams::Param param, int val) override;
    void createSettingUIMapping(MainWindow *mw);
    bool checkSettingsChanged(MainWindow *mw);
    void resetSettings(MainWindow *mw);
    void getChangedSettings(MainWindow *mw, QJsonObject& o, bool isNoCardInsterted);

};

#endif // SETTINGSGUIMINI_H
