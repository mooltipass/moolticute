#ifndef ISETTINGSGUI_H
#define ISETTINGSGUI_H

class QJsonObject;
class MainWindow;
namespace Ui {
class MainWindow;
}

class ISettingsGui
{
public:
    ISettingsGui(MainWindow* mw): m_mw{mw}{}
    virtual ~ISettingsGui(){};
    virtual void createSettingUIMapping() = 0;

protected:
    MainWindow* m_mw = nullptr;
    Ui::MainWindow* ui = nullptr;
};

#endif // ISETTINGSGUI_H
