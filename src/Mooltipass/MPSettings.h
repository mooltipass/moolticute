#ifndef MPSETTINGS_H
#define MPSETTINGS_H

#include <QObject>
#include "QtHelper.h"
#include "Common.h"
#include "MooltipassCmds.h"

class MPDevice;
class IMessageProtocol;
class WSServerCon;

class MPSettings
{

public:
    MPSettings(MPDevice *parent, IMessageProtocol *mesProt);
    virtual ~MPSettings();

    //reload parameters from MP
    virtual void loadParameters() = 0;
    virtual void updateParam(MPParams::Param param, int val) = 0;
    void updateParam(MPParams::Param param, bool en);

    virtual MPParams::Param getParamId(const QString& paramName) = 0;
    virtual QString getParamName(MPParams::Param param) = 0;
    virtual void sendEveryParameter() = 0;
    virtual void connectSendParams(QObject* slotObject) = 0;

protected:
    MPDevice* mpDevice = nullptr;
    IMessageProtocol* pMesProt = nullptr;

    //flag set when loading all parameters
    bool m_readingParams = false;
};

#endif // MPSETTINGS_H
