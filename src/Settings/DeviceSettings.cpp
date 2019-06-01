#include "DeviceSettings.h"

DeviceSettings::DeviceSettings(QObject *parent)
    : QObject(parent)
{
    fillParameterMapping();
}

MPParams::Param DeviceSettings::getParamId(const QString &paramName)
{
    return m_paramMap.key(paramName);
}

QString DeviceSettings::getParamName(MPParams::Param param)
{
    return m_paramMap[param];
}

void DeviceSettings::sendEveryParameter()
{
    //When called from a child class need to send parent's properties too
    auto* metaObj = metaObject();
    while (nullptr != metaObj && QString{metaObj->className()} != "QObject")
    {
        sendEveryParameter(metaObj);
        metaObj = metaObj->superClass();
    }

}

void DeviceSettings::connectSendParams(QObject *slotObject)
{
    //Connect every propertyChanged signals to the correct sendParams slot
    //When called from a child class need to connect parent's properties too
    auto* metaObj = metaObject();
    while (nullptr != metaObj && QString{metaObj->className()} != "QObject")
    {
        connectSendParams(slotObject, metaObj);
        metaObj = metaObj->superClass();
    }
}

void DeviceSettings::setProperty(const QString &propName, int value)
{
    bool propSet = false;
    auto* metaObj = metaObject();
    while (nullptr != metaObj && QString{metaObj->className()} != "QObject")
    {
        if (setProperty(propName, value, metaObj))
        {
            propSet = true;
            return;
        }
        metaObj = metaObj->superClass();
    }

    if (!propSet)
    {
        qWarning() << "Unable to set property: " << propName;
    }
}

void DeviceSettings::sendEveryParameter(const QMetaObject* meta)
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

void DeviceSettings::connectSendParams(QObject *slotObject, const QMetaObject* meta)
{
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i)
    {
        const auto signal = meta->property(i).notifySignal();
        const bool isFirstParamBool = signal.parameterType(0) == QVariant::Bool;
        const auto slot = slotObject->metaObject()->method(
                            slotObject->metaObject()->indexOfMethod(
                                isFirstParamBool ? "sendParams(bool,int)" : "sendParams(int,int)"
                          ));
        connect(this, signal, slotObject, slot);
    }
}

bool DeviceSettings::setProperty(const QString &propName, int value, const QMetaObject *meta)
{
    for (int i = meta->propertyOffset(); i < meta->propertyCount(); ++i)
    {
        if (meta->property(i).name() == propName)
        {
            meta->property(i).write(this, value);
            return true;
        }
    }
    return false;
}

void DeviceSettings::updateParam(MPParams::Param param, bool en)
{
    updateParam(param, static_cast<int>(en));
}

void DeviceSettings::fillParameterMapping()
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
