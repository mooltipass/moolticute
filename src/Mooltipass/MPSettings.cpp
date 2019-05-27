#include "MPSettings.h"
#include "AsyncJobs.h"
#include "MPDevice.h"
#include "IMessageProtocol.h"
#include "WSServerCon.h"

MPSettings::MPSettings(MPDevice *parent, IMessageProtocol *mesProt)
    : QObject(parent),
      mpDevice(parent),
      pMesProt(mesProt)
{
    fillParameterMapping();
}

MPSettings::~MPSettings()
{

}

MPParams::Param MPSettings::getParamId(const QString &paramName)
{
    return m_paramMap.key(paramName);
}

QString MPSettings::getParamName(MPParams::Param param)
{
    return m_paramMap[param];
}

void MPSettings::sendEveryParameter()
{
    sendEveryParameter(metaObject());
    //When called from a child class need to send parent's properties too
    const auto* superMetaObject = metaObject()->superClass();
    if (nullptr == superMetaObject)
    {
        return;
    }
    sendEveryParameter(superMetaObject);
}

void MPSettings::connectSendParams(WSServerCon *wsServerCon)
{
    //Connect every propertyChanged signals to the correct sendParams slot
    connectSendParams(wsServerCon, metaObject());
    //When called from a child class need to connect parent's properties too
    auto* superMetaObject = metaObject()->superClass();
    if (nullptr == superMetaObject)
    {
        return;
    }
    connectSendParams(wsServerCon, superMetaObject);
}

void MPSettings::fillParameterMapping()
{
    m_paramMap = {
        {MPParams::KEYBOARD_LAYOUT_PARAM, "keyboard_layout"},
        {MPParams::LOCK_TIMEOUT_ENABLE_PARAM, "lock_timeout_enabled"},
        {MPParams::LOCK_TIMEOUT_PARAM, "lock_timeout"},
        {MPParams::SCREENSAVER_PARAM, "screensaver"},
        {MPParams::USER_REQ_CANCEL_PARAM, "user_request_cancel"},
        {MPParams::USER_INTER_TIMEOUT_PARAM, "user_interaction_timeout"},
        {MPParams::FLASH_SCREEN_PARAM, "flash_screen"},
        {MPParams::OFFLINE_MODE_PARAM, "offline_mode"},
        {MPParams::TUTORIAL_BOOL_PARAM, "tutorial_enabled"},
        {MPParams::KEY_AFTER_LOGIN_SEND_BOOL_PARAM, "key_after_login_enabled"},
        {MPParams::KEY_AFTER_LOGIN_SEND_PARAM, "key_after_login"},
        {MPParams::KEY_AFTER_PASS_SEND_BOOL_PARAM, "key_after_pass_enabled"},
        {MPParams::KEY_AFTER_PASS_SEND_PARAM, "key_after_pass"},
        {MPParams::DELAY_AFTER_KEY_ENTRY_BOOL_PARAM, "delay_after_key_enabled"},
        {MPParams::DELAY_AFTER_KEY_ENTRY_PARAM, "delay_after_key"},
        {MPParams::RANDOM_INIT_PIN_PARAM, "random_starting_pin"}
    };
}

void MPSettings::sendEveryParameter(const QMetaObject* meta)
{
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i)
    {
        const auto signalName = meta->property(i).notifySignal().name();
        QVariant value = meta->property(i).read(this);
        if (value.type() == QVariant::Bool)
        {
            emit QMetaObject::invokeMethod(this, signalName, Q_ARG(bool, value.toBool()));
        }
        else
        {
            emit QMetaObject::invokeMethod(this, signalName, Q_ARG(int, value.toInt()));
        }
    }
}

void MPSettings::connectSendParams(WSServerCon *wsServerCon, const QMetaObject* meta)
{
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i)
    {
        const auto signal = meta->property(i).notifySignal();
        const bool isFirstParamBool = signal.parameterType(0) == QVariant::Bool;
        const auto slot = wsServerCon->metaObject()->method(
                            wsServerCon->metaObject()->indexOfMethod(
                                isFirstParamBool ? "sendParams(bool,int)" : "sendParams(int,int)"
                          ));
        connect(this, signal, wsServerCon, slot);
    }
}

void MPSettings::updateParam(MPParams::Param param, bool en)
{
    updateParam(param, static_cast<int>(en));
}
