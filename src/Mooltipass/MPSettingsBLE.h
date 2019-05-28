#ifndef MPSETTINGSBLE_H
#define MPSETTINGSBLE_H

#include "MPSettings.h"

class MPSettingsBLE : public MPSettings
{
    Q_OBJECT

    //MP BLE only
    QT_SETTINGS_PROPERTY(bool, reservedBle, false, MPParams::RESERVED_BLE)
    QT_SETTINGS_PROPERTY(bool, promptAnimation, false, MPParams::PROMPT_ANIMATION_PARAM)

public:
    MPSettingsBLE(MPDevice *parent, IMessageProtocol *mesProt);

    void loadParameters() override;
    void updateParam(MPParams::Param param, int val) override;

private slots:
    void setSettings();

private:
    void connectSendParams(WSServerCon* wsServerCon) override;
    void fillParameterMapping();

    QByteArray m_settings;
    QMap<MPParams::Param, int> m_bleByteMapping;

    static constexpr int RESERVED_BYTE = 0;
    static constexpr int RANDOM_PIN_BYTE = 1;
    static constexpr int USER_INTERACTION_TIMEOUT_BYTE = 2;
    static constexpr int ANIMATION_DURING_PROMPT_BYTE = 3;
};

#endif // MPSETTINGSBLE_H
