// Qt
#include <QDateTime>
#include <QMouseEvent>
#include <QDebug>

// Application
#include "CredentialView.h"
#include "CredentialModelFilter.h"
#include "CredentialModel.h"
#include "ServiceItem.h"
#include "ItemDelegate.h"
#include "TreeItem.h"
#include "LoginItem.h"

CredentialView::CredentialView(QWidget *parent) : QTreeView(parent),
    m_bIsFullyExpanded(false), m_pCurrentServiceItem(nullptr), m_pItemDelegate(nullptr)
{
    m_tSelectionTimer.setInterval(50);
    m_tSelectionTimer.setSingleShot(true);
    connect(&m_tSelectionTimer, &QTimer::timeout, this, &CredentialView::onSelectionTimerTimeOut);

    setHeaderHidden(true);
    m_pItemDelegate = new ItemDelegate(this);
    setItemDelegateForColumn(0, m_pItemDelegate);
    setMinimumWidth(430);
    connect(this, &CredentialView::clicked, this, &CredentialView::onToggleExpandedState);
}

CredentialView::~CredentialView()
{

}

void CredentialView::refreshLoginItem(const QModelIndex &srcIndex, bool bIsFavorite)
{
    Q_UNUSED(srcIndex)
    if (bIsFavorite)
        m_pItemDelegate->emitSizeHintChanged(srcIndex);
    else
        viewport()->update();
}

void CredentialView::onModelLoaded(bool bClearLoginDescription)
{
    Q_UNUSED(bClearLoginDescription)
    CredentialModelFilter *pCredModelFilter = dynamic_cast<CredentialModelFilter *>(model());

    QModelIndex firstServiceIndex = model()->index(0, 0, QModelIndex());
    if (firstServiceIndex.isValid())
    {
        bool wasAnyItemFavoriteExpanded = false;

        // Expand every index marked as favorite
        for (int i = 0; i < pCredModelFilter->rowCount(); i ++)
        {
            QModelIndex serviceIndex = pCredModelFilter->index(i, 0, QModelIndex());
            for (int j = 0; j < pCredModelFilter->rowCount(serviceIndex); j ++)
            {
                QModelIndex itemIndex = pCredModelFilter->index(j, 0, serviceIndex);
                TreeItem *pItem = pCredModelFilter->getItemByProxyIndex(itemIndex);
                LoginItem *loginItem = dynamic_cast<LoginItem *>(pItem);
                if (loginItem && loginItem->favorite() >= 0)
                {
                    expand(serviceIndex);
                    expand(itemIndex);
                    wasAnyItemFavoriteExpanded = true;
                }
            }
        }

        if (!wasAnyItemFavoriteExpanded) {
            selectionModel()->setCurrentIndex(firstServiceIndex, QItemSelectionModel::ClearAndSelect);
            onToggleExpandedState(firstServiceIndex);
        }
    }
}

void CredentialView::onToggleExpandedState(const QModelIndex &proxyIndex)
{
    CredentialModelFilter *pCredModelFilter = dynamic_cast<CredentialModelFilter *>(model());
    TreeItem *pItem = pCredModelFilter->getItemByProxyIndex(proxyIndex);
    ServiceItem *pServiceItem = dynamic_cast<ServiceItem *>(pItem);
    m_pCurrentServiceItem = nullptr;
    if (pServiceItem != nullptr)
    {
        m_pCurrentServiceItem = pServiceItem;
        if (!isExpanded(proxyIndex))
        {
            expand(proxyIndex);
            if (pServiceItem->childCount() > 0)
                m_tSelectionTimer.start();
        }
        else
            collapse(proxyIndex);
    }
}

void CredentialView::onChangeExpandedState()
{
    m_bIsFullyExpanded = !m_bIsFullyExpanded;
    if (m_bIsFullyExpanded)
        expandAll();
    else
        collapseAll();
    emit expandedStateChanged(m_bIsFullyExpanded);
}

void CredentialView::onSelectionTimerTimeOut()
{
    if ((m_pCurrentServiceItem != nullptr) && (m_pCurrentServiceItem->isExpanded()))
    {
        CredentialModelFilter *pCredModelFilter = dynamic_cast<CredentialModelFilter *>(model());
        CredentialModel *pCredModel = dynamic_cast<CredentialModel *>(pCredModelFilter->sourceModel());
        QModelIndex serviceIndex = pCredModel->getServiceIndexByName(m_pCurrentServiceItem->name());
        if (serviceIndex.isValid())
        {
            QModelIndex proxyIndex = pCredModelFilter->mapFromSource(serviceIndex);
            if (proxyIndex.isValid())
            {
                int nVisibleChilds = pCredModelFilter->rowCount(proxyIndex);
                if (nVisibleChilds > 0)
                {
                    QModelIndex firstLoginIndex = proxyIndex.child(0, 0);
                    if (firstLoginIndex.isValid())
                        setCurrentIndex(firstLoginIndex);
                }
            }
        }
    }
}
