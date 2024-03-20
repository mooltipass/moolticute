#ifndef CREDENTIALMODEL_H
#define CREDENTIALMODEL_H

// Qt
#include <QAbstractItemModel>
#include <QJsonArray>
#include <QDate>
#include <QTimer>
#include <QIcon>

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
        ItemNameRole=Qt::UserRole+1,
        PasswordRole,
        DescriptionRole,
        DateUpdatedRole,
        DateAccessedRole,
        FavoriteRole,
        CategoryRole,
        KeyAfterLoginRole,
        KeyAfterPwdRole
    };

    CredentialModel(QObject *parent=nullptr);
    ~CredentialModel();
    virtual QVariant data(const QModelIndex &idx, int role) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual Qt::ItemFlags flags(const QModelIndex &idx) const;
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &idx) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    void load(const QJsonArray &json, bool isFido = false);
    void setClearTextPassword(const QString &sServiceName, const QString &sLoginName, const QString &sPassword);
    QJsonArray getJsonChanges();
    void addCredential(QString sServiceName, const QString &sLoginName, const QString &sPassword,
                       const QString &sDescription = "", const QByteArray& pointedTo = QByteArray{});
    bool removeCredential(const QModelIndex &idx);
    TreeItem *getItemByIndex(const QModelIndex &idx) const;
    void updateLoginItem(const QModelIndex &idx, const QString &sPassword, const QString &sDescription, const QString &sName, int iCat, int iLoginKey, int iPwdKey);
    void updateLoginItem(const QModelIndex &idx, const ItemRole &role, const QVariant &vValue);
    void clear();
    QModelIndex getServiceIndexByName(const QString &sServiceName, int column = 0) const;
    QModelIndex getServiceIndexByNamePart(const QString &sServiceName, int column = 0) const;
    LoginItem *getLoginItemByIndex(const QModelIndex &idx) const;
    ServiceItem *getServiceItemByIndex(const QModelIndex &idx) const;
    QString getCategoryName(int catId) const;
    void updateCategories(const QString& cat1, const QString& cat2, const QString& cat3, const QString& cat4);
    bool isUserCategoryClean() const { return m_categoryClean; }
    void setUserCategoryClean(bool clean) { m_categoryClean = clean; }
    void setTOTP(const QModelIndex &idx, QString secretKey, int timeStep, int codeSize);
    QSet<qint8> getTakenFavorites() const;
    QString getCredentialNameForAddress(QByteArray addr) const;
    static QFont serviceFont();

private:
    ServiceItem *addService(const QString &sServiceName);
    qint8 getAvailableFavorite(qint8 newFav);
    QModelIndex getServiceIndex(const QString &sServiceName, Qt::MatchFlag flag, int column = 0) const;

private:
    RootItem *m_pRootItem;
    QList<QString> m_categories{tr("Default category"), "", "", "", ""};
    bool m_categoryClean = false;

signals:
    void modelLoaded(bool bClearLoginDescription);
    void selectLoginItem(LoginItem *pLoginItem);
};

#endif // CREDENTIALMODEL_H
