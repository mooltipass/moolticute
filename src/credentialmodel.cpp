// Qt
#include <QJsonArray>
#include <QJsonObject>
#include <QColor>
#include <QFont>

// Application
#include "credentialmodel.h"
#include "rootitem.h"
#include "serviceitem.h"
#include "loginitem.h"
#include "treeitem.h"
#include "AppGui.h"

CredentialModel::CredentialModel(QObject *parent) : QAbstractItemModel(parent)
{
    m_pRootItem = new RootItem();
}

CredentialModel::~CredentialModel()
{
    delete m_pRootItem;
}

QVariant CredentialModel::data(const QModelIndex &idx, int role) const
{
    // Get item (no need check index)
    TreeItem *pItem = getItemByIndex(idx);
    if (!pItem)
        return QVariant();

    // Cast to login item
    LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);

    // Display data for role
    if (role == Qt::DisplayRole)
    {
        QString sData = pItem->name();
        if (pLoginItem != nullptr)
        {
            QString sTargetDate = pLoginItem->createdDate().toString(Qt::DefaultLocaleShortDate);
            if (!pLoginItem->updatedDate().isNull())
                sTargetDate = pLoginItem->updatedDate().toString(Qt::DefaultLocaleShortDate);
            sData += QString(" (") + sTargetDate + QString(")");
        }
        return sData;
    }

    if (role == Qt::ForegroundRole)
    {
        if (pLoginItem != nullptr)
            return QColor("#3D96AF");
        return QColor("black");
    }

    if (role == Qt::FontRole)
    {
        QFont font;
        font.setBold(true);
        if (pLoginItem != nullptr)
            font.setItalic(true);
        return font;
    }

    if (role == Qt::DecorationRole) {
        if (pLoginItem != nullptr)
            return AppGui::qtAwesome()->icon(fa::arrowcircleright, {{ "color", QColor("#0097a7") },
                                                                    { "color-selected", QColor("#0097a7") },
                                                                    { "color-active", QColor("#0097a7") }});
    }

    return QVariant();
}

Qt::ItemFlags CredentialModel::flags(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return 0;

    return QAbstractItemModel::flags(idx);
}

QModelIndex CredentialModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *pParentItem = nullptr;

    if (!parent.isValid())
        pParentItem = m_pRootItem;
    else
        pParentItem = static_cast<TreeItem *>(parent.internalPointer());

    TreeItem *pChildItem = pParentItem->child(row);
    if (pChildItem)
        return createIndex(row, column, pChildItem);
    else
        return QModelIndex();
}

QModelIndex CredentialModel::parent(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return QModelIndex();

    TreeItem *pChildItem = static_cast<TreeItem *>(idx.internalPointer());
    TreeItem *pParentItem = pChildItem->parentItem();

    if (pParentItem == m_pRootItem)
        return QModelIndex();

    return createIndex(pParentItem->row(), 0, pParentItem);
}

int CredentialModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *pParentItem = nullptr;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        pParentItem = m_pRootItem;
    else
        pParentItem = static_cast<TreeItem *>(parent.internalPointer());

    return pParentItem->childCount();
}

int CredentialModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 1;
}

TreeItem *CredentialModel::getItemByIndex(const QModelIndex &idx) const
{
    if (!idx.isValid())
        return nullptr;
    return static_cast<TreeItem *>(idx.internalPointer());
}

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

    emit modelLoaded();
}

ServiceItem *CredentialModel::addService(const QString &sServiceName)
{
    // Do we have a match for sServiceName?
    ServiceItem *pServiceItem = m_pRootItem->findServiceByName(sServiceName);
    if (pServiceItem != nullptr)
        return pServiceItem;

    // Create new service
    return new ServiceItem(sServiceName);
}

QModelIndex CredentialModel::getServiceIndexByName(const QString &sServiceName) const
{
    QModelIndexList lMatches = match(index(0, 0, QModelIndex()), Qt::DisplayRole, sServiceName, 1);
    if (!lMatches.isEmpty())
        return lMatches.first();

    return QModelIndex();
}

void CredentialModel::updateLoginItem(const QModelIndex &idx, const QString &sPassword, const QString &sDescription, const QString &sName)
{
    // Retrieve item
    LoginItem *pLoginItem = dynamic_cast<LoginItem *>(getItemByIndex(idx));
    if (pLoginItem != nullptr)
    {
        updateLoginItem(idx, PasswordIdx, sPassword);
        updateLoginItem(idx, DescriptionIdx, sDescription);
        updateLoginItem(idx, LoginIdx, sName);
    }
}

void CredentialModel::updateLoginItem(const QModelIndex &idx, const ColumnIdx &colIdx, const QVariant &vValue)
{
    TreeItem *pItem = getItemByIndex(idx);
    LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);
    bool bChanged = false;
    if (pLoginItem != nullptr)
    {
        switch (colIdx)
        {
        case LoginIdx:
        {
            QString sName = vValue.toString();
            if (sName != pLoginItem->name())
            {
                pLoginItem->setName(sName);
                bChanged = true;
            }
            break;
        }
        case PasswordIdx:
        {
            QString sPassword = vValue.toString();
            if (sPassword != pLoginItem->password())
            {
                pLoginItem->setPassword(sPassword);
                bChanged = true;
            }
            break;
        }
        case DescriptionIdx:
        {
            QString sDescription = vValue.toString();
            if (sDescription != pLoginItem->description())
            {
                pLoginItem->setDescription(sDescription);
                bChanged = true;
            }
            break;
        }
        case DateCreatedIdx:
        {
            QDate dDate = vValue.toDate();
            if (dDate != pLoginItem->createdDate())
            {
                pLoginItem->setCreatedDate(dDate);
                bChanged = true;
            }
            break;
        }
        case DateModifiedIdx:
        {
            QDate dDate = vValue.toDate();
            if (dDate != pLoginItem->updatedDate())
            {
                pLoginItem->setUpdatedDate(dDate);
                bChanged = true;
            }
            break;
        }
        case FavoriteIdx:
        {
            qint8 iFavorite = (qint8)vValue.toInt();
            if (iFavorite != pLoginItem->favorite())
            {
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

void CredentialModel::clear()
{
    beginResetModel();
    m_pRootItem->clear();
    endResetModel();
}

void CredentialModel::setClearTextPassword(const QString &sServiceName, const QString &sLoginName, const QString &sPassword)
{
    // Retrieve target service
    ServiceItem *pTargetService = m_pRootItem->findServiceByName(sServiceName);

    // Got a match
    if (pTargetService != nullptr)
    {
        // Retrieve target login
        LoginItem *pTargetLogin = pTargetService->findLoginByName(sLoginName);
        if (pTargetLogin != nullptr)
        {
            pTargetLogin->setPassword(sPassword);
            pTargetLogin->setPasswordOrig(sPassword);
        }
    }
}

QJsonArray CredentialModel::getJsonChanges()
{
    QJsonArray jarr;
    QVector<TreeItem *> vServices = m_pRootItem->childs();
    foreach (TreeItem *pService, vServices)
    {
        QVector<TreeItem *> vLogins = pService->childs();
        foreach (TreeItem *pLogin, vLogins) {
            LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pLogin);
            QJsonObject object = pLoginItem->toJson();
            jarr.append(object);
        }
    }

    return jarr;
}

void CredentialModel::addCredential(const QString &sServiceName, const QString &sLoginName, const QString &sPassword, const QString &sDescription)
{
    // Retrieve target service
    ServiceItem *pTargetService = m_pRootItem->findServiceByName(sServiceName);

    // Check if a service/login pair already exists
    if (pTargetService != nullptr)
    {
        // Retrieve target login
        LoginItem *pTargetLogin = pTargetService->findLoginByName(sLoginName);
        if (pTargetLogin != nullptr)
            return;
        else
        {
            QModelIndex serviceIndex = getServiceIndexByName(pTargetService->name());
            if (serviceIndex.isValid())
            {
                qDebug() << " AVANT SERVICE CHILD COUNT " << pTargetService->childCount();

                beginInsertRows(serviceIndex, rowCount(serviceIndex), rowCount(serviceIndex));
                LoginItem *pLoginItem = pTargetService->addLogin(sLoginName);
                pLoginItem->setPassword(sPassword);
                pLoginItem->setDescription(sDescription);
                endInsertRows();

                qDebug() << " AVANT SERVICE CHILD COUNT " << pTargetService->childCount();
             }
        }
    }
    // No matching service
    else
    {
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        ServiceItem *pServiceItem = m_pRootItem->addService(sServiceName);
        LoginItem *pLoginItem = pServiceItem->addLogin(sLoginName);
        pLoginItem->setPassword(sPassword);
        pLoginItem->setDescription(sDescription);
        endInsertRows();
    }
}

void CredentialModel::removeCredential(const QModelIndex &idx)
{
    if (idx.isValid())
    {
        TreeItem *pItem = getItemByIndex(idx);
        if (pItem == nullptr)
            return;

        // Check what we have
        ServiceItem *pServiceItem = dynamic_cast<ServiceItem *>(pItem);
        LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);
        if ((pServiceItem == nullptr) && (pLoginItem == nullptr))
            return;

        // Retrieve parent item
        TreeItem *pParentItem = pItem->parentItem();
        if (pParentItem == nullptr)
            return;

        // Can remove a service only if it has no child
        bool bCondition = true;
        if (pServiceItem != nullptr)
            bCondition = (pServiceItem->childCount() == 0);
        QModelIndex parentIndex = idx.parent();
        if (bCondition)
        {
            beginRemoveRows(parentIndex, idx.row(), idx.row());
            if (pParentItem->removeOne(pItem))
                delete pItem;
            endRemoveRows();
        }

        if (pParentItem && (pParentItem != m_pRootItem) && (pParentItem->childCount() == 0))
            removeCredential(parentIndex);
    }
}

