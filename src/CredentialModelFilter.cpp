// Application
#include "CredentialModelFilter.h"
#include "CredentialModel.h"
#include "LoginItem.h"

CredentialModelFilter::CredentialModelFilter(QObject *parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

CredentialModelFilter::~CredentialModelFilter()
{

}

void CredentialModelFilter::setFilter(const QString &sFilter)
{
    m_sFilter = sFilter;
    invalidateFilter();
}

bool CredentialModelFilter::switchFavFilter()
{
    m_favFilter = !m_favFilter;
    invalidateFilter();
    return m_favFilter;
}

TreeItem *CredentialModelFilter::getItemByProxyIndex(const QModelIndex &proxyIndex)
{
    QModelIndex srcIndex = mapToSource(proxyIndex);
    CredentialModel *pSrcModel = dynamic_cast<CredentialModel *>(sourceModel());
    return pSrcModel->getItemByIndex(srcIndex);
}

const TreeItem *CredentialModelFilter::getItemByProxyIndex(const QModelIndex &proxyIndex) const
{
    QModelIndex srcIndex = mapToSource(proxyIndex);
    CredentialModel *pSrcModel = dynamic_cast<CredentialModel *>(sourceModel());
    return pSrcModel->getItemByIndex(srcIndex);
}

void CredentialModelFilter::sort(int column, Qt::SortOrder order)
{
    // sort order is not passed as a parameter to lessThan so need
    // to save sort order globally.
    tempSortOrder = order;
    return QSortFilterProxyModel::sort(column, order);
}

QModelIndexList CredentialModelFilter::getNextRow(const QModelIndex& rowIdx)
{
    QModelIndexList nextRow;
    QModelIndex parent = rowIdx.parent();
    if (!rowIdx.isValid())
        return nextRow;

    if (rowCount(parent) > rowIdx.row()+1)
    {
        nextRow << index(rowIdx.row()+1, 0, parent);
        nextRow << index(rowIdx.row()+1, 1, parent);
    }
    else if (parent.isValid())
    {
        if  (rowCount(parent.parent()) > parent.row()+1)
        {
            QModelIndex nextParent = index(parent.row()+1, 0, parent.parent());
            nextRow << index(0, 0, nextParent);
            nextRow << index(0, 1, nextParent);
        }
        else if (rowIdx.row() > 0)
        {
            nextRow << index(rowIdx.row()-1, 0, parent);
            nextRow << index(rowIdx.row()-1, 1, parent);

        }
        else if (parent.row() > 0)
        {
            QModelIndex prevousParent = index(parent.row()-1, 0, parent.parent());
            nextRow << index(rowCount(prevousParent)-1, 0, prevousParent);
            nextRow << index(rowCount(prevousParent)-1, 1, prevousParent);
        }
    }

    return nextRow;
}

void CredentialModelFilter::refreshFavorites()
{
    if (m_favFilter)
    {
        invalidateFilter();
    }
}

bool CredentialModelFilter::filterAcceptsRow(int iSrcRow, const QModelIndex &srcParent) const
{
    // Get source index
    QModelIndex srcIndex = sourceModel()->index(iSrcRow, 0, srcParent);
    if (srcIndex.isValid())
    {
        // If any of children matches the filter, then current index matches the filter as well
        int iRowCount = sourceModel()->rowCount(srcIndex);
        for (int i=0; i<iRowCount; ++i)
            if (filterAcceptsRow(i, srcIndex))
                return true;
        return acceptRow(iSrcRow, srcParent);
    }

    // Parent call for initial behaviour:
    return QSortFilterProxyModel::filterAcceptsRow(iSrcRow, srcParent) ;
}

bool CredentialModelFilter::testItemAgainstNameAndDescription(TreeItem *pItem, const QString &sFilter) const
{
    if (pItem)
    {
        bool bCondition1 = pItem->name().contains(sFilter, Qt::CaseInsensitive);
        bool bCondition2 = pItem->description().contains(sFilter, Qt::CaseInsensitive);
        return bCondition1 || bCondition2;
    }
    return false;
}

bool CredentialModelFilter::acceptRow(int iSrcRow, const QModelIndex &srcParent) const
{
    // Get src model
    CredentialModel *pCredentialModel  = dynamic_cast<CredentialModel *>(sourceModel());

    // Get src index
    QModelIndex srcIndex = pCredentialModel->index(iSrcRow, 0, srcParent);
    if (srcIndex.isValid())
    {
        // Get item
        TreeItem *pItem = pCredentialModel->getItemByIndex(srcIndex);
        if (pItem != nullptr)
        {
            // Is it a login item?
            LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);

            bool shouldDisplayByFavFilter = true;
            if (m_favFilter)
            {
                if (pLoginItem != nullptr)
                {
                    shouldDisplayByFavFilter = !m_favFilter || (pLoginItem->favorite() != -1 && m_favFilter);
                }
                else
                {
                    shouldDisplayByFavFilter = !m_favFilter;
                }
            }

            if (pLoginItem != nullptr)
            {
                TreeItem *pParentItem = pLoginItem->parentItem();
                bool bCondition = testItemAgainstNameAndDescription(pParentItem, m_sFilter) && shouldDisplayByFavFilter;
                if (bCondition)
                    return true;
            }

            return testItemAgainstNameAndDescription(pItem, m_sFilter) && shouldDisplayByFavFilter;
        }
    }

    return true;
}

bool CredentialModelFilter::lessThan(const QModelIndex &srcLeft, const QModelIndex &srcRight) const
{
    // Get src model
    CredentialModel *pSrcModel = dynamic_cast<CredentialModel *>(sourceModel());

    TreeItem *pLeftItem = pSrcModel->getItemByIndex(srcLeft);
    TreeItem *pRightItem = pSrcModel->getItemByIndex(srcRight);

    if ((pLeftItem != nullptr) && (pRightItem != nullptr))
    {
        if (srcLeft.column() == 0 && srcRight.column() == 0)
        {
            return pLeftItem->name() < pRightItem->name();
        }

        if ((srcLeft.column() == 1 && srcRight.column() == 1))
        {
            return pLeftItem->bestUpdateDate(tempSortOrder)
                        < pRightItem->bestUpdateDate(tempSortOrder);
        }
    }

    return false;
}

QModelIndex CredentialModelFilter::getProxyIndexFromItem(TreeItem *pItem, int column)
{
    CredentialModel *pCredModel = dynamic_cast<CredentialModel *>(sourceModel());
    switch (pItem->treeType())
    {
    case TreeItem::TreeType::Service:
    {
        QModelIndex serviceIndex = pCredModel->getServiceIndexByName(pItem->name(), column);
        return mapFromSource(serviceIndex);
    }
    case TreeItem::TreeType::Login:
    {
        QModelIndex parentIndex = getProxyIndexFromItem(pItem->parentItem());

        for (int i = 0; i < rowCount(parentIndex); i ++)
        {
            QModelIndex loginIndex = index(i, column, parentIndex);
            if (loginIndex.isValid())
            {
                TreeItem *pLoginItem = getItemByProxyIndex(loginIndex);
                if (pLoginItem->name() == pItem->name())
                    return loginIndex;
            }
        }
        return QModelIndex();
    }
    case TreeItem::TreeType::Root:
    default:
        return QModelIndex();
    }
}
