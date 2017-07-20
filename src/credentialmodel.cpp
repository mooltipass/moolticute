// Qt
#include <QJsonArray>
#include <QJsonObject>

// Application
#include "credentialmodel.h"
#include "rootitem.h"
#include "serviceitem.h"
#include "loginitem.h"
#include "treeitem.h"

//-------------------------------------------------------------------------------------------------

CredentialModel::CredentialModel(QObject *parent) : QAbstractItemModel(parent)
{
    m_pRootItem = new RootItem();
}

//-------------------------------------------------------------------------------------------------

CredentialModel::~CredentialModel()
{

}

//-------------------------------------------------------------------------------------------------

QVariant CredentialModel::data(const QModelIndex &idx, int role) const
{
    // Check index
    if (!idx.isValid())
        return QVariant();

    // Check row
    if ((idx.row() < 0) || (idx.row() > (rowCount()-1)))
        return QVariant();

    // Get item
    TreeItem *pItem = getItemByIndex(idx);
    if (!pItem)
        return QVariant();

    // Cast to login item
    LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);

    // Display data for role
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (idx.column()) {
        case ServiceIdx: return pItem->name();
        case LoginIdx: return pItem->name();
        case PasswordIdx: return (pLoginItem != nullptr) ? pLoginItem->password() : QVariant();
        case DescriptionIdx: return pItem->description();
        case DateCreatedIdx: {
            if (pItem->createdDate().isNull())
                return tr("N/A");
            return pItem->createdDate();
        }
        case DateModifiedIdx: {
            if (pItem->updatedDate().isNull())
                return tr("N/A");
            return pItem->updatedDate();
        }
        case FavoriteIdx: (pLoginItem != nullptr) ? pLoginItem->favorite() : QVariant();
        }
    }

    if (role == LoginRole && idx.column() == ServiceIdx)
        return (pLoginItem != nullptr) ? pLoginItem->name() : QVariant();

    if (role == FavRole && idx.column() == ServiceIdx)
        return (pLoginItem != nullptr) ? pLoginItem->favorite() : QVariant();

    if (role == PasswordUnlockedRole && idx.column() == PasswordIdx)
        return (pLoginItem != nullptr) ? !pLoginItem->password().isEmpty() : false;

    if (role == UidRole)
        return pItem->uid();

    return QVariant();
}

//-------------------------------------------------------------------------------------------------

Qt::ItemFlags CredentialModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return 0;

    return QAbstractItemModel::flags(idx);
}

//-------------------------------------------------------------------------------------------------

QModelIndex CredentialModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *pParentItem = !parent.isValid() ? m_pRootItem : static_cast<TreeItem *>(parent.internalPointer());
    TreeItem *pChildItem = pParentItem ? pParentItem->child(row) : nullptr;
    return pChildItem ? createIndex(row, column, pChildItem) : QModelIndex();
}

//-------------------------------------------------------------------------------------------------

QModelIndex CredentialModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QModelIndex();

    TreeItem *pChildItem = static_cast<TreeItem *>(idx.internalPointer());
    if (!pChildItem)
        return QModelIndex();
    TreeItem *pParentItem = pChildItem->parentItem();

    return pParentItem == m_pRootItem ? QModelIndex() : createIndex(pParentItem->row(), 0, pParentItem);
}

//-------------------------------------------------------------------------------------------------

int CredentialModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0)
        return 0;
    TreeItem *pParentItem = !parent.isValid() ? m_pRootItem : static_cast<TreeItem *>(parent.internalPointer());
    return pParentItem ? pParentItem->childCount() : 0;
}

//-------------------------------------------------------------------------------------------------

int CredentialModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

//-------------------------------------------------------------------------------------------------

TreeItem *CredentialModel::getItemByIndex(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return nullptr;
    return static_cast<TreeItem *>(idx.internalPointer());
}

//-------------------------------------------------------------------------------------------------

void CredentialModel::load(const QJsonArray &json)
{
    if (json.isEmpty())
        return;

    beginResetModel();

    m_pRootItem->setItemsStatus(TreeItem::UNUSED);
    for (int i=0; i<json.size(); i++)
    {
        QJsonObject pnode = json.at(i).toObject();

        QJsonArray jchilds = pnode["childs"].toArray();
        for (int j=0; j<jchilds.size(); j++)
        {
            QJsonObject cnode = jchilds.at(j).toObject();

            // Retrieve credential data
            QString sServiceName = pnode["service"].toString();

            // Check if this service already exists
            ServiceItem *pServiceItem = m_pRootItem->findServiceByName(sServiceName);

            // Service does not exist, add it
            if (pServiceItem == nullptr)
                pServiceItem = m_pRootItem->addService(sServiceName);
            pServiceItem->setStatus(TreeItem::USED);

            QString sLoginName = cnode["login"].toString();

            // Check if this login already exists
            LoginItem *pLoginItem = pServiceItem->findLoginByName(sLoginName);

            // Login does not exist, add it
            if (pLoginItem == nullptr)
                pLoginItem = pServiceItem->addLogin(sLoginName);
            pLoginItem->setStatus(TreeItem::USED);

            // Update login item description
            QString sDescription = cnode["description"].toString();
            pLoginItem->setDescription(sDescription);

            // Update login item created date
            QDate dCreatedDate = QDate::fromString(cnode["date_created"].toString(), Qt::ISODate);
            pLoginItem->setCreatedDate(dCreatedDate);

            // Update login item updated date
            QDate dUpdatedDate = QDate::fromString(cnode["date_last_used"].toString(), Qt::ISODate);
            pLoginItem->setUpdatedDate(dUpdatedDate);

            QJsonArray a = cnode["address"].toArray();
            if (a.size() < 2)
            {
                qWarning() << "Moolticute daemon did not send the node address, please upgrade moolticute daemon.";
                continue;
            }
            QByteArray bAddress;
            bAddress.append((char)a.at(0).toInt());
            bAddress.append((char)a.at(1).toInt());

            // Update login item address
            pLoginItem->setAddress(bAddress);

            // Update login favorite
            int iFavorite = cnode["favorite"].toInt();
            pLoginItem->setFavorite(iFavorite);
        }
    }

    // Remove unused items
    m_pRootItem->removeUnusedItems();

    endResetModel();
}

//-------------------------------------------------------------------------------------------------

ServiceItem *CredentialModel::addService(const QString &sServiceName)
{
    // Do we have a match for sServiceName?
    ServiceItem *pServiceItem = m_pRootItem->findServiceByName(sServiceName);
    if (pServiceItem != nullptr)
        return pServiceItem;

    // Create new service
    return new ServiceItem(sServiceName);
}

//-------------------------------------------------------------------------------------------------

QModelIndex CredentialModel::getItemIndexByUID(const QString &sItemUID) const
{
    QModelIndexList lMatches = match(index(0, 0, QModelIndex()), UidRole, sItemUID, 1, Qt::MatchRecursive);
    if (!lMatches.isEmpty())
        return lMatches.first();

    return QModelIndex();
}

//-------------------------------------------------------------------------------------------------

TreeItem *CredentialModel::getItemByUID(const QString &sItemUID) const
{
    QModelIndex itemIndex = getItemIndexByUID(sItemUID);
    if (itemIndex.isValid())
        return getItemByIndex(itemIndex);
    return nullptr;
}

//-------------------------------------------------------------------------------------------------

void CredentialModel::updateLoginItem(const QModelIndex &idx, const QString &sPassword, const QString &sDescription, const QString &sName)
{
    // Retrieve item
    LoginItem *pLoginItem = dynamic_cast<LoginItem *>(getItemByIndex(idx));
    if (pLoginItem != nullptr) {
        updateLoginItem(idx, PasswordIdx, sPassword);
        updateLoginItem(idx, DescriptionIdx, sDescription);
        updateLoginItem(idx, LoginIdx, sName);
    }
}

//-------------------------------------------------------------------------------------------------

void CredentialModel::updateLoginItem(const QModelIndex &idx, const ColumnIdx &colIdx, const QVariant &vValue)
{
    TreeItem *pItem = getItemByIndex(idx);
    LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);
    bool bChanged = false;
    if (pLoginItem != nullptr) {
        switch (colIdx) {
        case LoginIdx: {
            QString sName = vValue.toString();
            if (sName != pLoginItem->name()) {
                pLoginItem->setName(sName);
                bChanged = true;
            }
            break;
        }
        case PasswordIdx: {
            QString sPassword = vValue.toString();
            if (sPassword != pLoginItem->password()) {
                pLoginItem->setPassword(sPassword);
                bChanged = true;
            }
            break;
        }
        case DescriptionIdx: {
            QString sDescription = vValue.toString();
            if (sDescription != pLoginItem->description()) {
                pLoginItem->setDescription(sDescription);
                bChanged = true;
            }
            break;
        }
        case DateCreatedIdx: {
            QDate dDate = vValue.toDate();
            if (dDate != pLoginItem->createdDate()) {
                pLoginItem->setCreatedDate(dDate);
                bChanged = true;
            }
            break;
        }
        case DateModifiedIdx: {
            QDate dDate = vValue.toDate();
            if (dDate != pLoginItem->updatedDate()) {
                pLoginItem->setUpdatedDate(dDate);
                bChanged = true;
            }
            break;
        }
        case FavoriteIdx: {
            qint8 iFavorite = (qint8)vValue.toInt();
            if (iFavorite != pLoginItem->favorite()) {
                pLoginItem->setFavorite(iFavorite);
                bChanged = true;
            }
            break;
        }
        default: break;
        }
        if (bChanged)
            emit dataChanged(idx, idx);
    }
}

//-------------------------------------------------------------------------------------------------

void CredentialModel::setClearTextPassword(const QString &sServiceName, const QString &sLoginName, const QString &sPassword)
{
    // Retrieve target service
    ServiceItem *pTargetService = m_pRootItem->findServiceByName(sServiceName);

    // Got a match
    if (pTargetService != nullptr) {
        // Retrieve target login
        LoginItem *pTargetLogin = pTargetService->findLoginByName(sLoginName);
        if (pTargetLogin != nullptr) {
            pTargetLogin->setPassword(sPassword);
            pTargetLogin->setPasswordOrig(sPassword);
            QModelIndex itemIndex = getItemIndexByUID(pTargetLogin->uid());
            if (itemIndex.isValid())
                emit dataChanged(itemIndex, itemIndex);
        }
    }
}

//-------------------------------------------------------------------------------------------------

void CredentialModel::update(const QString &sServiceName, const QString &sLoginName, const QString &sPassword, const QString &sDescription)
{
    // Retrieve target service
    ServiceItem *pTargetService = m_pRootItem->findServiceByName(sServiceName);
    if (pTargetService != nullptr) {
        // Retrieve target login
        LoginItem *pTargetLogin = pTargetService->findLoginByName(sLoginName);
        if (pTargetLogin != nullptr) {
            if (!sDescription.isEmpty())
                pTargetLogin->setDescription(sDescription);
            pTargetLogin->setPassword(sPassword);
            QModelIndex itemIndex = getItemIndexByUID(pTargetLogin->uid());
            if (itemIndex.isValid())
                emit dataChanged(itemIndex, itemIndex);
        }
        else {
            QModelIndex serviceIndex = getItemIndexByUID(pTargetService->uid());
            if (serviceIndex.isValid()) {
                beginInsertRows(serviceIndex, pTargetService->childCount(), pTargetService->childCount());
                LoginItem *pLoginItem = pTargetService->addLogin(sLoginName);
                if (!sDescription.isEmpty())
                    pLoginItem->setDescription(sDescription);
                pLoginItem->setPassword(sPassword);
                endInsertRows();
            }
        }
    }
    else {
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        ServiceItem *pServiceItem = m_pRootItem->addService(sServiceName);
        LoginItem *pLoginItem = pServiceItem->addLogin(sLoginName);
        if (!sDescription.isEmpty())
            pLoginItem->setDescription(sDescription);
        pLoginItem->setPassword(sPassword);
        endInsertRows();
    }
}

//-------------------------------------------------------------------------------------------------

void CredentialModel::update(const QModelIndex &idx, const LoginItem *pRefLoginItem)
{
    if (pRefLoginItem != nullptr) {
        if (!idx.isValid()) {
            // Retrieve parent
            TreeItem *pParentItem = pRefLoginItem->parentItem();
            if (pParentItem != nullptr)
                update(pParentItem->name(), pRefLoginItem->name(), pRefLoginItem->password(), pRefLoginItem->description());
        }
        else
        {
            // Retrieve item
            TreeItem *pItem = getItemByIndex(idx);
            LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);
            if (pLoginItem != nullptr) {
                pLoginItem->setDescription(pRefLoginItem->description());
                pLoginItem->setFavorite(pRefLoginItem->favorite());
                pLoginItem->setName(pRefLoginItem->name());
                pLoginItem->setPassword(pRefLoginItem->password());
                QModelIndex itemIndex = getItemIndexByUID(pLoginItem->uid());
                if (itemIndex.isValid())
                    emit dataChanged(itemIndex, itemIndex);
            }
        }
    }
}

//-------------------------------------------------------------------------------------------------

QJsonArray CredentialModel::getJsonChanges()
{
    QJsonArray jarr;
    QVector<TreeItem *> vServices = m_pRootItem->childs();
    foreach (TreeItem *pService, vServices) {
        QJsonArray addr;
        ServiceItem *pServiceItem = dynamic_cast<ServiceItem *>(pService);
        QVector<TreeItem *> vLogins = pServiceItem->childs();
        foreach (TreeItem *pLogin, vLogins) {
            LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pLogin);
            QByteArray sAddress = pLoginItem->address();
            if (!sAddress.isEmpty())
            {
                addr.append((int)sAddress.at(0));
                addr.append((int)sAddress.at(1));
            }
            QString p;
            QString sPassword = pLoginItem->password();
            QString sPasswordOrig = pLoginItem->passwordOrig();
            if (!sPasswordOrig.isEmpty())
            {
                if (sPassword != sPasswordOrig)
                    p = sPassword;
            }
            QJsonObject object
            {{"service", pServiceItem->name()},
                {"login", pLoginItem->name()},
                {"password", p},
                {"description", pLoginItem->description()},
                {"address", addr},
                {"favorite", pLoginItem->favorite()}};

            jarr.append(object);
        }
    }
    return jarr;
}

//-------------------------------------------------------------------------------------------------

void CredentialModel::removeCredential(const QModelIndex &idx)
{
    if (idx.isValid()) {
        TreeItem *pItem = getItemByIndex(idx);
        if (pItem != nullptr) {
            TreeItem *pParentItem = pItem->parentItem();
            if (pParentItem != nullptr) {
                beginRemoveRows(idx.parent(), idx.row(), idx.row());
                if (pParentItem->removeOne(pItem))
                    delete pItem;
                endRemoveRows();
            }
        }
    }
}
