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
    enum ItemRole
    {
        ItemNameRole,
        ItemLabelRole,
        PasswordRole,
        DescriptionRole,
        DateCreatedRole,
        DateUpdatedRole,
        FavoriteRole
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
    void updateLoginItem(const QModelIndex &idx, const QString &sPassword, const QString &sDescription, const QString &sName);
    void updateLoginItem(const QModelIndex &idx, const ItemRole &role, const QVariant &vValue);
    void clear();

private:
    ServiceItem *addService(const QString &sServiceName);
    QModelIndex getServiceIndexByName(const QString &sServiceName) const;

private:
    RootItem *m_pRootItem;

signals:
    void modelLoaded();
};

#endif // CREDENTIALMODEL_H
