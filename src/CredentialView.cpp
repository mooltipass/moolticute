 // Qt
#include <QDateTime>
#include <QMouseEvent>
#include <QDebug>
#include <QHeaderView>

// Application
#include "CredentialView.h"
#include "CredentialModelFilter.h"
#include "CredentialModel.h"
#include "ServiceItem.h"
#include "ItemDelegate.h"
#include "TreeItem.h"
#include "LoginItem.h"

CredentialView::CredentialView(QWidget *parent)
    : QTreeView(parent)
  , m_bIsFullyExpanded(false)
  , m_pCurrentServiceItem(nullptr)
  , m_pCurrentLoginItem(nullptr)
  , m_pItemDelegate(nullptr)
{
    m_tSelectionTimer.setInterval(50);
    m_tSelectionTimer.setSingleShot(true);
    connect(&m_tSelectionTimer, &QTimer::timeout, this, &CredentialView::onSelectionTimerTimeOut);

    setSortingEnabled(true);
    m_pItemDelegate = new ItemDelegate(this);
    setItemDelegateForColumn(0, m_pItemDelegate);
    setMinimumWidth(430);
    connect(this, &CredentialView::clicked, this, &CredentialView::onToggleExpandedState, Qt::DirectConnection);

    header()->setStyleSheet("QHeaderView::section{ background-color: #D5D5D5; }");
    setColumnWidth(0, 300);
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
    LoginItem* ploginItem = dynamic_cast<LoginItem *>(pItem);    
    Q_ASSERT((pServiceItem != nullptr || ploginItem != nullptr));

    m_pCurrentServiceItem = nullptr;
    m_pCurrentLoginItem = nullptr;
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
        {
            collapse(proxyIndex);
        }
    }
    else if (ploginItem != nullptr)
    {
        QModelIndex proxyParent = proxyIndex.parent();
        Q_ASSERT(proxyParent.isValid());
        TreeItem *pParent = pCredModelFilter->getItemByProxyIndex(proxyParent);
        ServiceItem *pServiceItem = dynamic_cast<ServiceItem *>(pParent);
        m_pCurrentServiceItem = pServiceItem;
        m_pCurrentLoginItem = ploginItem;       
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
    if ((m_pCurrentServiceItem != nullptr))
    {
        CredentialModelFilter *pCredModelFilter = dynamic_cast<CredentialModelFilter *>(model());
        CredentialModel *pCredModel = dynamic_cast<CredentialModel *>(pCredModelFilter->sourceModel());
        QModelIndex serviceIndex = pCredModel->getServiceIndexByName(m_pCurrentServiceItem->name());
        if (serviceIndex.isValid())
        {
            QModelIndex proxyServiceIndex = pCredModelFilter->mapFromSource(serviceIndex);
            if (proxyServiceIndex.isValid())
            {
                if (m_pCurrentServiceItem->isExpanded())
                {
                    int nVisibleChilds = pCredModelFilter->rowCount(proxyServiceIndex);
                    if (nVisibleChilds > 0)
                    {
                        QModelIndex firstLoginIndex = proxyServiceIndex.child(0, 0);
                        if (firstLoginIndex.isValid())
                        {
                            setCurrentIndex(firstLoginIndex);

                            TreeItem *pItem = pCredModelFilter->getItemByProxyIndex(firstLoginIndex);
                            LoginItem* ploginItem = dynamic_cast<LoginItem *>(pItem);
                            m_pCurrentLoginItem = ploginItem;
                        }
                    }
                }
                else
                {
                    m_pCurrentLoginItem = nullptr;
                    setCurrentIndex(proxyServiceIndex);
                }

            }
        }
    }
}

void CredentialView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    setColumnWidth(0, 300);
    connect(model, &QAbstractItemModel::layoutChanged, this, &CredentialView::onLayoutChanged);
}


void CredentialView::onLayoutChanged(const QList<QPersistentModelIndex> &parents
                                     , QAbstractItemModel::LayoutChangeHint hint)
{
    Q_UNUSED(parents)
    if (QAbstractItemModel::VerticalSortHint == hint && tempTreeItem)
    {
        onSelectionTimerTimeOut();
    }
}

void CredentialView::dumpCurrentItem(const QString& intro)
{
    CredentialModelFilter *pCredModelFilter = dynamic_cast<CredentialModelFilter *>(model());
    TreeItem* currentItem = pCredModelFilter->getItemByProxyIndex(currentIndex());
    qDebug() << intro
             << "\tService\t" << (m_pCurrentServiceItem ? m_pCurrentServiceItem->name() : "none")
             << "\tLogin\t" << (m_pCurrentLoginItem ? m_pCurrentLoginItem->name() : "none")
             << "\tcurrent\t" << (currentItem ? currentItem->name() : "none");
}

