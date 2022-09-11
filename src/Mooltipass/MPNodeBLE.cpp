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

    return (data.size() == PARENT_NODE_LENGTH && (NodeParent == type || NodeParentData == type)) ||
            (data.size() == CHILD_NODE_LENGTH && (NodeChild == type || NodeChildData == type));
}

QByteArray MPNodeBLE::getLoginNodeData() const
{
    // return core data, excluding linked lists, flags and last child node used address
    if (!isValid()) return QByteArray();
    return data.mid(DATA_ADDR_START, LAST_CHILD_NODE_USED_ADDR_START - DATA_ADDR_START);
}

void MPNodeBLE::setLoginNodeData(const QByteArray &flags, const QByteArray &d)
{
    // overwrite core data, excluding linked lists and last child node used address
    if (isValid())
    {
        const auto parentNodeSize = pMesProt->getParentNodeSize();
        const auto bytesAfterLastUsed = parentNodeSize - LAST_CHILD_NODE_USED_ADDR_START;
        data.replace(DATA_ADDR_START, parentNodeSize - DATA_ADDR_START - bytesAfterLastUsed, d);
        data.replace(0, ADDRESS_LENGTH, flags);
    }
}

QByteArray MPNodeBLE::getLoginChildNodeData() const
{
    // return core data, excluding linked lists, flags and pointed to child address
    if (!isValid()) return QByteArray();
    return data.mid(LOGIN_CHILD_NODE_DATA_ADDR_START);
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
    if (category > 0)
    {
        categoryBit |= (0b1 << (category - 1));
    }
    data[0] = static_cast<char>(categoryBit);
}

int MPNodeBLE::getKeyAfterLogin() const
{
    if (!isValid()) return 0;
    return pMesProt->toIntFromLittleEndian(data[KEY_AFTER_LOGIN_ADDR_START], data[KEY_AFTER_LOGIN_ADDR_START+1]);
}

void MPNodeBLE::setKeyAfterLogin(int key)
{
    if (isValid())
    {
        const auto keyArray = pMesProt->toLittleEndianFromInt(key);
        data[KEY_AFTER_LOGIN_ADDR_START] = keyArray[0];
        data[KEY_AFTER_LOGIN_ADDR_START+1] = keyArray[1];
    }
}

int MPNodeBLE::getKeyAfterPwd() const
{
    if (!isValid()) return 0;
    return pMesProt->toIntFromLittleEndian(data[KEY_AFTER_PWD_ADDR_START], data[KEY_AFTER_PWD_ADDR_START+1]);
}

void MPNodeBLE::setKeyAfterPwd(int key)
{
    if (isValid())
    {
        const auto keyArray = pMesProt->toLittleEndianFromInt(key);
        data[KEY_AFTER_PWD_ADDR_START] = keyArray[0];
        data[KEY_AFTER_PWD_ADDR_START+1] = keyArray[1];
    }
}

int MPNodeBLE::getPwdBlankFlag() const
{
    if (!isValid()) return 0;
    return data[PWD_BLANK_FLAG];
}

void MPNodeBLE::setPwdBlankFlag()
{
    if (isValid())
    {
        data[PWD_BLANK_FLAG] = static_cast<char>(BLANK_CHAR);
    }
}

int MPNodeBLE::getTOTPTimeStep() const
{
    if (!isValid()) return 0;
    return data[TOTP_TIME_STEP];
}

int MPNodeBLE::getTOTPCodeSize() const
{
    if (!isValid()) return 0;
    return data[TOTP_CODE_SIZE];
}

void MPNodeBLE::resetTOTPCredential()
{
    for (int i = TOTP_ADDR_START; i < TOTP_ADDR_START + TOTP_LENGTH; ++i)
    {
        data[i] = static_cast<char>(0x00);
    }
}

void MPNodeBLE::setPointedToChildAddr(const QByteArray &d, const quint32 virt_addr)
{
    if (d.isNull())
    {
        pointedToChildVirtualAddr = virt_addr;
        pointedToChildVirtualAddrSet = true;
    }
    else
    {
        pointedToChildVirtualAddrSet = false;
        data[POINTED_TO_CHILD_START] = d[0];
        data[POINTED_TO_CHILD_START+1] = d[1];
    }
}

quint32 MPNodeBLE::getPointedToChildVirtualAddress() const
{
    return pointedToChildVirtualAddr;
}

QByteArray MPNodeBLE::getPointedToChildAddr() const
{
    if (!isValid()) return QByteArray();
    if (nextVirtualAddressSet) return QByteArray();
    return data.mid(POINTED_TO_CHILD_START, ADDRESS_LENGTH);
}

QByteArray MPNodeBLE::getLastChildNodeUsedAddr() const
{
    if (!isValid()) return QByteArray();
    if (nextVirtualAddressSet) return QByteArray();
    return data.mid(LAST_CHILD_NODE_USED_ADDR_START, ADDRESS_LENGTH);
}

void MPNodeBLE::setLastChildNodeUsedAddr(const QByteArray &d)
{
    if (d.size() < 2)
    {
        qCritical() << "Invalid address for setLastChildNodeUsedAddr";
        return;
    }
    data[LAST_CHILD_NODE_USED_ADDR_START] = d[0];
    data[LAST_CHILD_NODE_USED_ADDR_START + 1] = d[1];
}

QString MPNodeBLE::getMultipleDomains() const
{
    QString ret = "";
    if (getType() != NodeParent)
    {
        qCritical() << "Multiple domains is only set for node parents!";
        return ret;
    }
    if (data[0] & (1 << MULTIPLE_DOMAINS_BIT))
    {
        qCritical() << "Multiple domain is set";
        ret = pMesProt->toQString(data.mid(MULTIPLE_DOMAINS_START, MULTIPLE_DOMAINS_LENGTH));
        qCritical() << "Multiple domains: " << ret;
    }
    return ret;
}

bool MPNodeBLE::setMultipleDomains(const QString &domains)
{
    bool changed = false;
    QString oldDomains = getMultipleDomains();
    if (domains.isEmpty())
    {
        if (!oldDomains.isEmpty())
        {
            // If old domains were not empty, need to unset the flag
            quint8 flag = data[0];
            flag &= ~(1 << MULTIPLE_DOMAINS_BIT);
            data[0] = static_cast<char>(flag);
            // And fill domains bytes with zero bytes
            QByteArray zeroBytes(MULTIPLE_DOMAINS_LENGTH, Common::ZERO_BYTE);
            data.replace(MULTIPLE_DOMAINS_START, MULTIPLE_DOMAINS_LENGTH, zeroBytes);
            changed = true;
        }
    }
    else
    {
        if (domains != oldDomains)
        {
            // If domains value is changed, need to replace the old value with the new bytes
            QByteArray domainsBytes = pMesProt->toByteArray(domains);
            const auto domainsSize = domainsBytes.size();
            if (domainsSize > MULTIPLE_DOMAINS_LENGTH)
            {
                qCritical() << "Multiple domains length is longer than allowed!";
                domainsBytes.resize(MULTIPLE_DOMAINS_LENGTH);
            } else if (domainsSize < MULTIPLE_DOMAINS_LENGTH)
            {
                Common::fill(domainsBytes, MULTIPLE_DOMAINS_LENGTH - domainsSize, Common::ZERO_BYTE);
            }
            data.replace(MULTIPLE_DOMAINS_START, MULTIPLE_DOMAINS_LENGTH, domainsBytes);
        }
        if (oldDomains.isEmpty())
        {
            // If old domains were empty, need to set the multiple domains bit
            quint8 flag = data[0];
            flag |= 1 << MULTIPLE_DOMAINS_BIT;
            data[0] = static_cast<char>(flag);
        }
        changed = true;
    }
    return changed;
}
