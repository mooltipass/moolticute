// Qt
#include <QJsonArray>
#include <QDebug>

// Application
#include "loginitem.h"
#include "serviceitem.h"

LoginItem::LoginItem(const QString &sLoginName) : TreeItem(sLoginName),
    m_iFavorite(-1), m_sPassword(""), m_sPasswordOrig("")
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

    qDebug() << "*** INPUT PASSWORDS ARE *** " << m_sPasswordOrig << m_sPassword;

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
