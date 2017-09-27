// Qt
#include <QJsonArray>
#include <QDebug>

// Application
#include "LoginItem.h"
#include "ServiceItem.h"

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

const QString &LoginItem::passwordOrig() const
{
    return m_sPasswordOrig;
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
    return {{ "service", service },
            { "login", m_sName },
            { "password", p },
            { "description", m_sDescription },
            { "address", addr },
            { "favorite", m_iFavorite }};
}

QString LoginItem::itemLabel() const
{
    QString sTargetDate = createdDate().toString(Qt::DefaultLocaleShortDate);
    if (!updatedDate().isNull())
        sTargetDate = updatedDate().toString(Qt::DefaultLocaleShortDate);
    return m_sName + QString(" (") + sTargetDate + QString(")");
}

void LoginItem::setPasswordLocked(bool bLocked)
{
    m_bPasswordLocked = bLocked;
}

bool LoginItem::passwordLocked() const
{
    return m_bPasswordLocked;
}
