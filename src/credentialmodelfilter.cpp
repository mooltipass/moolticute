// Application
#include "credentialmodelfilter.h"
#include "credentialmodel.h"
#include "serviceitem.h"
#include "loginitem.h"

//-------------------------------------------------------------------------------------------------

CredentialModelFilter::CredentialModelFilter(QObject *parent) : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

//-------------------------------------------------------------------------------------------------

CredentialModelFilter::~CredentialModelFilter()
{

}

//-------------------------------------------------------------------------------------------------

void CredentialModelFilter::setFilter(const QString &sFilter)
{
    m_sFilter = sFilter.toLower();
    invalidate();
}

//-------------------------------------------------------------------------------------------------

int CredentialModelFilter::indexToSource(int idx)
{
    return mapToSource(index(idx, 0)).row();
}

//-------------------------------------------------------------------------------------------------

int CredentialModelFilter::indexFromSource(int idx)
{
    return mapFromSource(index(idx, 0)).row();
}

//-------------------------------------------------------------------------------------------------

bool CredentialModelFilter::filterAcceptsRow(int iSrcRow, const QModelIndex &srcParent) const
{
    // get source-model index for current row
    QModelIndex source_index = sourceModel()->index(iSrcRow, 0, srcParent);
    if (source_index.isValid())
    {
        // if any of children matches the filter, then current index matches the filter as well
        int iRowCount = sourceModel()->rowCount(source_index) ;
        for (int i=0; i<iRowCount; ++i)
            if (filterAcceptsRow(i, source_index))
                return true ;
        return acceptRow(iSrcRow, srcParent);
    }

    // Parent call for initial behaviour:
    return QSortFilterProxyModel::filterAcceptsRow(iSrcRow, srcParent) ;
}

// Accept row:
bool CredentialModelFilter::acceptRow(int iSrcRow, const QModelIndex &srcParent) const
{
    // Get src model
    CredentialModel *pCredentialModel  = dynamic_cast<CredentialModel *>(sourceModel());

    // Get src index
    QModelIndex srcIndex = pCredentialModel->index(iSrcRow, 0, srcParent);
    if (srcIndex.isValid()) {
        // Get item
        TreeItem *pItem = pCredentialModel->getItem(srcIndex);
        if (pItem != nullptr) {
            ServiceItem *pServiceItem = dynamic_cast<ServiceItem *>(pItem);
            if (pServiceItem != nullptr)
                return pServiceItem->name().contains(m_sFilter, Qt::CaseInsensitive);
            LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);
            if (pLoginItem != nullptr) {
                bool bCondition1 = pLoginItem->name().contains(m_sFilter, Qt::CaseInsensitive);
                bool bCondition2 = pLoginItem->description().contains(m_sFilter, Qt::CaseInsensitive);
                return bCondition1 || bCondition2;
            }
        }
    }

    return true;
}

//-------------------------------------------------------------------------------------------------

bool CredentialModelFilter::lessThan(const QModelIndex &srcLeft, const QModelIndex &srcRight) const
{
    // Get src model
    CredentialModel *pSrcModel = dynamic_cast<CredentialModel *>(sourceModel());

    TreeItem *pLeftItem = pSrcModel->getItem(srcLeft);
    TreeItem *pRightItem = pSrcModel->getItem(srcRight);

    if ((pLeftItem != nullptr) && (pRightItem != nullptr))
        return pLeftItem->name() < pRightItem->name();

    return false;
}

