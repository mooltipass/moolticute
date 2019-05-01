#include "MPNodeBLE.h"


MPNodeBLE::MPNodeBLE(const QByteArray &d, QObject *parent, const QByteArray &nodeAddress, const quint32 virt_addr)
    : MPNode(d, parent, nodeAddress, virt_addr)
{
}

MPNodeBLE::MPNodeBLE(QObject *parent, const QByteArray &nodeAddress, const quint32 virt_addr)
    : MPNode(parent, nodeAddress, virt_addr)
{
}

MPNodeBLE::MPNodeBLE(QByteArray &&d, QObject *parent, QByteArray &&nodeAddress, const quint32 virt_addr)
    : MPNode(qMove(d), parent, qMove(nodeAddress), virt_addr)
{
}

MPNodeBLE::MPNodeBLE(QObject *parent, QByteArray &&nodeAddress, const quint32 virt_addr)
    : MPNode(parent, qMove(nodeAddress), virt_addr)
{
}

bool MPNodeBLE::isDataLengthValid() const
{
    return isValid();
}

bool MPNodeBLE::isValid() const
{
    if (NodeUnknown == getType())
    {
        return false;
    }

    return (data.size() == PARENT_NODE_LENGTH || data.size() == CHILD_NODE_LENGTH);
}

QString MPNodeBLE::getService() const
{
    if (!isValid()) return QString();
    return pMesProt->toQString(data.mid(SERVICE_ADDR_START, SERVICE_LENGTH));
}

void MPNodeBLE::setService(const QString &service)
{
    if (isValid())
    {
        QByteArray serviceArray = pMesProt->toByteArray(service);
        serviceArray.append('\0');
        serviceArray.resize(SERVICE_LENGTH);
        serviceArray[serviceArray.size()-1] = '\0';
        data.replace(SERVICE_ADDR_START, SERVICE_LENGTH, serviceArray);
    }
}

QByteArray MPNodeBLE::getStartDataCtr() const
{
    if (!isValid()) return QByteArray();
    return data.mid(CTR_DATA_ADDR_START, CTR_LENGTH);
}

QByteArray MPNodeBLE::getCTR() const
{
    if (!isValid()) return QByteArray();
    return data.mid(CTR_ADDR_START, CTR_LENGTH);
}

QString MPNodeBLE::getDescription() const
{
    if (!isValid()) return QString();
    return pMesProt->toQString(data.mid(DESC_ADDR_START, DESC_LENGTH));
}

void MPNodeBLE::setDescription(const QString &newDescription)
{
    if (isValid())
    {
        QByteArray desc = pMesProt->toByteArray(newDescription);
        desc.append('\0');
        desc.resize(DESC_LENGTH);
        desc[desc.size()-1] = '\0';
        data.replace(DESC_ADDR_START, DESC_LENGTH, desc);
    }
}

QString MPNodeBLE::getLogin() const
{
    if (!isValid()) return QString();
    return pMesProt->toQString(data.mid(LOGIN_ADDR_START, LOGIN_LENGTH));
}

void MPNodeBLE::setLogin(const QString &newLogin)
{
    if (isValid())
    {
        QByteArray login = pMesProt->toByteArray(newLogin);
        login.append('\0');
        login.resize(LOGIN_LENGTH);
        login[login.size()-1] = '\0';
        data.replace(LOGIN_ADDR_START, LOGIN_LENGTH, login);
    }
}

QByteArray MPNodeBLE::getPasswordEnc() const
{
    if (!isValid()) return QByteArray();
    return data.mid(PWD_ENC_ADDR_START, PWD_ENC_LENGTH);
}

QDate MPNodeBLE::getDateCreated() const
{
    if (!isValid()) return QDate();
    return Common::bytesToDate(data.mid(DATE_CREATED_ADDR_START, ADDRESS_LENGTH));
}

QDate MPNodeBLE::getDateLastUsed() const
{
    if (!isValid()) return QDate();
    return Common::bytesToDate(data.mid(DATE_LASTUSED_ADDR_START, ADDRESS_LENGTH));
}
