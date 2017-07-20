#ifndef LOGINITEM_H
#define LOGINITEM_H

// Qt
#include <QDate>

// Application
#include "treeitem.h"
class ServiceItem;

class LoginItem : public TreeItem
{
public:
    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //! Constructor
    LoginItem(const QString &sLoginName);

    //! Destructor
    virtual ~LoginItem();

    //-------------------------------------------------------------------------------------------------
    // Getters and setters
    //-------------------------------------------------------------------------------------------------

    //! Return address
    const QByteArray &address() const;

    //! Set address
    void setAddress(const QByteArray &bAddress);

    //! Return favorite
    qint8 favorite() const;

    //! Set favorite
    void setFavorite(qint8 iFavorite);

    //! Return password
    const QString &password() const;

    //! Set password
    void setPassword(const QString &sPassword);

    //! Return password orig
    const QString &passwordOrig() const;

    //! Set password orig
    void setPasswordOrig(const QString &setPassword);

private:    
    //! Address
    QByteArray m_bAddress;

    //! Favorite?
    qint8 m_iFavorite;

    //! Password
    QString m_sPassword;

    //! Password orig
    QString m_sPasswordOrig;
};

#endif // LOGINITEM_H
