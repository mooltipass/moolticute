#ifndef SETTINGSGUIHELPER_H
#define SETTINGSGUIHELPER_H

#include <QObject>
#include <QJsonObject>
#include "DeviceSettings.h"
#include "WSClient.h"

class MainWindow;

class SettingsGuiHelper : public QObject
{
    Q_OBJECT
public:
    explicit SettingsGuiHelper(WSClient* parent = nullptr);
    void createSettingUIMapping(MainWindow *mw);
    bool checkSettingsChanged();
    void resetSettings();
    void getChangedSettings(QJsonObject& o);
    void updateParameters(const QJsonObject &data);

private:
    WSClient* m_wsClient = nullptr;
    DeviceSettings* m_settings = nullptr;
    MainWindow* m_mw = nullptr;

    Common::MPHwVersion m_deviceType = Common::MP_Unknown;
};

#endif // SETTINGSGUIHELPER_H
