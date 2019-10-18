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
    const auto type = getType();
    if (NodeUnknown == type)
    {
        return false;
    }

    return (data.size() == PARENT_NODE_LENGTH && NodeParent == type) ||
           (data.size() == CHILD_NODE_LENGTH && NodeChild == type);
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

int MPNodeBLE::getCategory() const
{
    if (!isValid())
    {
        return 0;
    }

    /**
     * Category bitfield is the first 4 bit of
     * the 2 Bytes Long Flags in the child node
     * 0000 is category 0, 0001 is category 1,
     * 0010 is category 2
     */
    int categoryBit = data[0]&0xF;
    if (categoryBit < 4)
    {
        return categoryBit;
    }

    // 0100 is category 3
    if (0b100 == categoryBit)
    {
        return 3;
    }

    // 1000 is category 4
    if (0b1000 == categoryBit)
    {
        return 4;
    }

    qCritical() << "Invalid bitfield for category";
    return 0;
}

void MPNodeBLE::setCategory(int category)
{
    quint8 categoryBit = data[0]&0x0F0;
    categoryBit |= category;
    data[0] = static_cast<char>(categoryBit);
}
