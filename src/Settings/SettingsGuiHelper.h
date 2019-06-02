#ifndef SETTINGSGUIHELPER_H
#define SETTINGSGUIHELPER_H

#include <QObject>
#include <QJsonObject>
#include "DeviceSettings.h"

class MainWindow;

class SettingsGuiHelper : public QObject
{
    Q_OBJECT
public:
    explicit SettingsGuiHelper(QObject *parent = nullptr);
    void createSettingUIMapping(MainWindow *mw);
    bool checkSettingsChanged();
    void resetSettings();
    void getChangedSettings(QJsonObject& o);
    void updateParameters(const QJsonObject &data);

private:
    DeviceSettings* m_settings = nullptr;
    MainWindow* m_mw = nullptr;

};

#endif // SETTINGSGUIHELPER_H
