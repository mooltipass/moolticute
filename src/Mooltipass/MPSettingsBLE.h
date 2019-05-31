#ifndef MPSETTINGSBLE_H
#define MPSETTINGSBLE_H

#include "MPSettings.h"
#include "DeviceSettingsBLE.h"

class MPSettingsBLE : public DeviceSettingsBLE, public MPSettings
{
    Q_OBJECT

public:
    MPSettingsBLE(MPDevice *parent, IMessageProtocol *mesProt);

    void loadParameters() override;
    void updateParam(MPParams::Param param, int val) override;

private slots:
    void setSettings();

private:
    void connectSendParams(WSServerCon* wsServerCon);

    QByteArray m_lastDeviceSettings;
};

#endif // MPSETTINGSBLE_H
