#ifndef MPSETTINGS_H
#define MPSETTINGS_H

#include <QObject>
#include "QtHelper.h"
#include "Common.h"
#include "MooltipassCmds.h"

class MPDevice;
class IMessageProtocol;
class WSServerCon;

template<typename T>
class MPSettings : public T
{
    Q_OBJECT

public:
    MPSettings(MPDevice *parent, IMessageProtocol *mesProt)
        : T(parent),
        mpDevice(parent),
        pMesProt(mesProt){}

    virtual ~MPSettings();

    //reload parameters from MP
    virtual void loadParameters() = 0;
    virtual void updateParam(MPParams::Param param, int val) = 0;
    void updateParam(MPParams::Param param, bool en)
    {
        updateParam(param, static_cast<int>(en));
    }

protected:
    MPDevice* mpDevice = nullptr;
    IMessageProtocol* pMesProt = nullptr;

    //flag set when loading all parameters
    bool m_readingParams = false;
};

#endif // MPSETTINGS_H
