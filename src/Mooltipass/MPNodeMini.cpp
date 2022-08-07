#include "MPNodeMini.h"


MPNodeMini::MPNodeMini(const QByteArray &d, QObject *parent, const QByteArray &nodeAddress, const quint32 virt_addr)
    : MPNode(d, parent, nodeAddress, virt_addr)
{
}

MPNodeMini::MPNodeMini(QObject *parent, const QByteArray &nodeAddress, const quint32 virt_addr)
    : MPNode(parent, nodeAddress, virt_addr)
{
}

MPNodeMini::MPNodeMini(QByteArray &&d, QObject *parent, QByteArray &&nodeAddress, const quint32 virt_addr)
    : MPNode(qMove(d), parent, qMove(nodeAddress), virt_addr)
{
}

MPNodeMini::MPNodeMini(QObject *parent, QByteArray &&nodeAddress, const quint32 virt_addr)
    : MPNode(parent, qMove(nodeAddress), virt_addr)
{
}

bool MPNodeMini::isDataLengthValid() const
{
    return data.size() == MP_NODE_SIZE;
}

bool MPNodeMini::isValid() const
{
    if (NodeUnknown == getType())
    {
        return false;
    }

    return data.size() == MP_NODE_SIZE &&
            (static_cast<quint8>(data[1]) & 0x20) == 0;
}

QByteArray MPNodeMini::getLoginNodeData() const
{
    // return core data, excluding linked lists and flags
    if (!isValid()) return QByteArray();
    return data.mid(DATA_ADDR_START);
}

void MPNodeMini::setLoginNodeData(const QByteArray &flags, const QByteArray &d)
{
    // overwrite core data, excluding linked lists
    if (isValid())
    {
        data.replace(DATA_ADDR_START, pMesProt->getParentNodeSize()-DATA_ADDR_START, d);
        data.replace(0, ADDRESS_LENGTH, flags);
    }
}

QByteArray MPNodeMini::getLoginChildNodeData() const
{
    // return core data, excluding linked lists and flags
    if (!isValid()) return QByteArray();
    return data.mid(LOGIN_CHILD_NODE_DATA_ADDR_START);
}

QString MPNodeMini::getService() const
{
    if (!isValid()) return QString();
    return pMesProt->toQString(data.mid(SERVICE_ADDR_START, MP_NODE_SIZE - 8 - 3));
}

void MPNodeMini::setService(const QString &service)
{
    if (isValid())
    {
        QByteArray serviceArray = pMesProt->toByteArray(service);
        serviceArray.append('\0');
        serviceArray.resize(MP_MAX_PAYLOAD_LENGTH);
        serviceArray[serviceArray.size()-1] = '\0';
        data.replace(SERVICE_ADDR_START, MP_MAX_PAYLOAD_LENGTH, serviceArray);
    }
}

QByteArray MPNodeMini::getStartDataCtr() const
{
    if (!isValid()) return QByteArray();
    return data.mid(CTR_DATA_ADDR_START, CTR_LENGTH);
}

QByteArray MPNodeMini::getCTR() const
{
    if (!isValid()) return QByteArray();
    return data.mid(CTR_ADDR_START, CTR_LENGTH);
}

QString MPNodeMini::getDescription() const
{
    if (!isValid()) return QString();
    return pMesProt->toQString(data.mid(DESC_ADDR_START, DESC_LENGTH));
}

void MPNodeMini::setDescription(const QString &newDescription)
{
    if (isValid())
    {
        QByteArray desc = pMesProt->toByteArray(newDescription);
        desc.append('\0');
        desc.resize(MP_MAX_DESC_LENGTH);
        desc[desc.size()-1] = '\0';
        data.replace(DESC_ADDR_START, MP_MAX_DESC_LENGTH, desc);
    }
}

QString MPNodeMini::getLogin() const
{
    if (!isValid()) return QString();
    return pMesProt->toQString(data.mid(LOGIN_ADDR_START, LOGIN_LENGTH));
}

void MPNodeMini::setLogin(const QString &newLogin)
{
    if (isValid())
    {
        QByteArray login = pMesProt->toByteArray(newLogin);
        login.append('\0');
        login.resize(MP_MAX_PAYLOAD_LENGTH);
        login[login.size()-1] = '\0';
        data.replace(LOGIN_ADDR_START, MP_MAX_PAYLOAD_LENGTH, login);
    }
}

QByteArray MPNodeMini::getPasswordEnc() const
{
    if (!isValid()) return QByteArray();
    return data.mid(PWD_ENC_ADDR_START, PWD_ENC_LENGTH);
}

QDate MPNodeMini::getDateCreated() const
{
    if (!isValid()) return QDate();
    return Common::bytesToDate(data.mid(DATE_CREATED_ADDR_START, ADDRESS_LENGTH));
}

QDate MPNodeMini::getDateLastUsed() const
{
    if (!isValid()) return QDate();
    return Common::bytesToDate(data.mid(DATE_LASTUSED_ADDR_START, ADDRESS_LENGTH));
}
