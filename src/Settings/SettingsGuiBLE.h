#ifndef SETTINGSGUIBLE_H
#define SETTINGSGUIBLE_H

#include <QObject>
#include "DeviceSettingsBLE.h"

class MainWindow;

class SettingsGuiBLE : public DeviceSettingsBLE
{
    Q_OBJECT
public:
    explicit SettingsGuiBLE(QObject *parent = nullptr);
    void loadParameters() override;
    void updateParam(MPParams::Param param, int val) override;
    void createSettingUIMapping(MainWindow *mw);
    bool checkSettingsChanged();
    void getChangedSettings(QJsonObject& o);

private:
    MainWindow* m_mw = nullptr;

};

#endif // SETTINGSGUIBLE_H
