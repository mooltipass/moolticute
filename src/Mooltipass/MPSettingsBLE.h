#ifndef MPSETTINGSBLE_H
#define MPSETTINGSBLE_H

#include "DeviceSettingsBLE.h"

class MPDevice;
class IMessageProtocol;

class MPSettingsBLE : public DeviceSettingsBLE
{
    Q_OBJECT

public:
    MPSettingsBLE(MPDevice *parent, IMessageProtocol *mesProt);
    virtual ~MPSettingsBLE() override = default;

    void loadParameters() override;
    void updateParam(MPParams::Param param, int val) override;

private slots:
    void setSettings();

private:
    void connectSendParams(QObject* slotObject) override;

    MPDevice* mpDevice = nullptr;
    IMessageProtocol* pMesProt = nullptr;

    QByteArray m_lastDeviceSettings;
};

#endif // MPSETTINGSBLE_H
