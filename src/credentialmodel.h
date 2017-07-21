#ifndef CREDENTIALMODEL_H
#define CREDENTIALMODEL_H

// Qt
#include <QAbstractItemModel>
#include <QJsonArray>
#include <QDate>

// Application
#include "Common.h"
class RootItem;
class ServiceItem;
class LoginItem;
class TreeItem;

class CredentialModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum ColumnIdx
    {
        ServiceIdx,
        LoginIdx,
        PasswordIdx,
        DescriptionIdx,
        DateCreatedIdx,
        DateModifiedIdx,
        FavoriteIdx,
        ColumnCount
    };

    enum CustomRole {
        LoginRole = Qt::UserRole + 1,
        PasswordUnlockedRole,
        FavRole,
        UidRole
    };

    class Credential
    {
    public:
        QString service;
        QString login;
        QString password, passwordOrig;
        QString description;
        QDate createdDate;
        QDate updatedDate;
        QByteArray address;
        qint8 favorite = Common::FAV_NOT_SET;

        bool operator==(const Credential &other) const
        {
            if (address.isEmpty() || other.address.isEmpty())
            {
                //if address is defined for both, check equality of address
                return address == other.address;
            }
            return service == other.service && login == other.login;
        }
    };

    CredentialModel(QObject *parent=nullptr);
    virtual ~CredentialModel();
    virtual QVariant data(const QModelIndex &idx, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &idx) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &idx) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    void load(const QJsonArray &json);
    void setClearTextPassword(const QString &sServiceName, const QString &sLoginName, const QString &sPassword);
    QJsonArray getJsonChanges();
    void addCredential(const QString &sServiceName, const QString &sLoginName, const QString &sPassword, const QString &sDescription="");
    void removeCredential(const QModelIndex &idx);
    TreeItem *getItemByIndex(const QModelIndex &idx) const;
    TreeItem *getItemByUID(const QString &sItemUID) const;
    void updateLoginItem(const QModelIndex &idx, const QString &sPassword, const QString &sDescription, const QString &sName);
    void updateLoginItem(const QModelIndex &idx, const ColumnIdx &colIdx, const QVariant &vValue);
    void clear();

private:
    ServiceItem *addService(const QString &sServiceName);
    QModelIndex getItemIndexByUID(const QString &sItemUID) const;

private:
    RootItem *m_pRootItem;

signals:
    void modelLoaded();
};

#endif // CREDENTIALMODEL_H
