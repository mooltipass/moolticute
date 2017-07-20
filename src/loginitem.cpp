// Qt
#include <QUuid>

// Application
#include "loginitem.h"
#include "serviceitem.h"

//-------------------------------------------------------------------------------------------------

LoginItem::LoginItem(const QString &sLoginName) : TreeItem(sLoginName),
    m_iFavorite(0), m_sPassword(""), m_sPasswordOrig("")
{

}

//-------------------------------------------------------------------------------------------------

LoginItem::~LoginItem()
{

}

//-------------------------------------------------------------------------------------------------

const QByteArray &LoginItem::address() const
{
    return m_bAddress;
}

//-------------------------------------------------------------------------------------------------

void LoginItem::setAddress(const QByteArray &bAddress)
{
    m_bAddress = bAddress;
}

//-------------------------------------------------------------------------------------------------

qint8 LoginItem::favorite() const
{
    return m_iFavorite;
}

//-------------------------------------------------------------------------------------------------

void LoginItem::setFavorite(qint8 iFavorite)
{
    m_iFavorite = iFavorite;
}

//-------------------------------------------------------------------------------------------------

const QString &LoginItem::password() const
{
    return m_sPassword;
}

//-------------------------------------------------------------------------------------------------

void LoginItem::setPassword(const QString &sPassword)
{
    m_sPassword = sPassword;
}

//-------------------------------------------------------------------------------------------------

const QString &LoginItem::passwordOrig() const
{
    return m_sPasswordOrig;
}

//-------------------------------------------------------------------------------------------------

void LoginItem::setPasswordOrig(const QString &setPassword)
{
    m_sPasswordOrig = setPassword;
}
