// Qt
#include <QJsonArray>
#include <QDebug>

// Application
#include "LoginItem.h"
#include "ServiceItem.h"
#include "DeviceDetector.h"

LoginItem::LoginItem(const QString &sLoginName) : TreeItem(sLoginName),
    m_iFavorite(-1), m_sPassword(""), m_sPasswordOrig(""), m_bPasswordLocked(true)
{
}

LoginItem::~LoginItem()
{
}

const QByteArray &LoginItem::address() const
{
    return m_bAddress;
}

void LoginItem::setAddress(const QByteArray &bAddress)
{
    m_bAddress = bAddress;
}

qint8 LoginItem::favorite() const
{
    return m_iFavorite;
}

void LoginItem::setFavorite(qint8 iFavorite)
{
    m_iFavorite = iFavorite;
}

const QString &LoginItem::password() const
{
    return m_sPassword;
}

void LoginItem::setPassword(const QString &sPassword)
{
    m_sPassword = sPassword;
}

void LoginItem::setPasswordOrig(const QString &setPassword)
{
    m_sPasswordOrig = setPassword;
}

QJsonObject LoginItem::toJson() const
{
    QJsonArray addr;
    if (!m_bAddress.isEmpty())
    {
        addr.append((int)m_bAddress.at(0));
        addr.append((int)m_bAddress.at(1));
    }

    QString p;
    //Only send password if it has been changed by user
    //Else the fiels stays empty
    //if (!m_sPasswordOrig.isEmpty())
    {
        if (m_sPassword != m_sPasswordOrig)
            p = m_sPassword;
    }

    QString service = (m_pParentItem != nullptr) ? m_pParentItem->name() : "";
    QJsonObject ret = {{ "service", service },
                       { "login", m_sName },
                       { "password", p },
                       { "description", m_sDescription },
                       { "address", addr },
                       { "favorite", m_iFavorite }
               };

    if (DeviceDetector::instance().isBle())
    {
        ret.insert("category", m_iCategory);
        ret.insert("key_after_login", m_iKeyAfterLogin);
        ret.insert("key_after_pwd", m_iKeyAfterPwd);
        if (m_totpCred.valid)
        {
            QJsonObject totp;
            totp["totp_secret_key"] = m_totpCred.secretKey;
            totp["totp_time_step"] = m_totpCred.timeStep;
            totp["totp_code_size"] = m_totpCred.codeSize;
            if (m_totpDeleted)
            {
                totp["totp_deleted"] = m_totpDeleted;
            }
            ret.insert("totp", totp);
        }
    }
    return ret;
}

void LoginItem::setPasswordLocked(bool bLocked)
{
    m_bPasswordLocked = bLocked;
}

bool LoginItem::passwordLocked() const
{
    return m_bPasswordLocked;
}

bool LoginItem::hasBlankPwdChanged() const
{
    return 0x00 != m_iPwdBlankFlag && !m_sPassword.isEmpty();
}

TreeItem::TreeType LoginItem::treeType() const
{
    return Login;
}

void LoginItem::setTOTPCredential(QString secretKey, int timeStep, int codeSize)
{
    m_totpCred = TOTPCredential{secretKey, timeStep, codeSize};
}
