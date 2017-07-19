// Qt
#include <QUuid>

// Application
#include "loginitem.h"
#include "serviceitem.h"

//-------------------------------------------------------------------------------------------------

LoginItem::LoginItem(const QString &sLoginName) : TreeItem(LOGIN, sLoginName),
    m_dCreatedDate(QDate::currentDate()),
    m_dUpdatedDate(QDate::currentDate()),
    m_sDescription(""),
    m_sPassword(""),
    m_sPasswordOrig("")
{

}

//-------------------------------------------------------------------------------------------------

LoginItem::~LoginItem()
{

}

//-------------------------------------------------------------------------------------------------

const QDate &LoginItem::createdDate() const
{
    return m_dCreatedDate;
}

//-------------------------------------------------------------------------------------------------

void LoginItem::setCreatedDate(const QDate &dDate)
{
    m_dCreatedDate = dDate;
}

//-------------------------------------------------------------------------------------------------

const QDate &LoginItem::updatedDate() const
{
    return m_dUpdatedDate;
}

//-------------------------------------------------------------------------------------------------

void LoginItem::setUpdatedDate(const QDate &dDate)
{
    m_dUpdatedDate = dDate;
}

//-------------------------------------------------------------------------------------------------

const QString &LoginItem::description() const
{
    return m_sDescription;
}

//-------------------------------------------------------------------------------------------------

void LoginItem::setDescription(const QString &sDescription)
{
    m_sDescription = sDescription;
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
