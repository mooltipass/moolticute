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
    void updateUI() override;
    void setupKeyboardLayout(bool onlyCheck) override;


    static const int TAB_INDEX = 43;
    static const int ENTER_INDEX = 40;
    static const int SPACE_INDEX = 44;
};

#endif // SETTINGSGUIMINI_H
