// Qt
#include <QDateTime>
#include <QMouseEvent>
#include <QDebug>

// Application
#include "credentialview.h"
#include "CredentialModelFilter.h"
#include "ServiceItem.h"
#include "ItemDelegate.h"

ConditionalItemSelectionModel::ConditionalItemSelectionModel(TestFunction f, QAbstractItemModel *model)
    : QItemSelectionModel(model)
    , cb(f)
    , lastRequestTime(0)
{

}

ConditionalItemSelectionModel::~ConditionalItemSelectionModel()
{

}

void ConditionalItemSelectionModel::select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command)
{
    if (canChangeIndex()) {
        QItemSelectionModel::select(index, command);
    }
}

void ConditionalItemSelectionModel::select(const QItemSelection & selection, QItemSelectionModel::SelectionFlags command)
{
    if (canChangeIndex())
        QItemSelectionModel::select(selection, command);
}

void  ConditionalItemSelectionModel::setCurrentIndex(const QModelIndex & index, QItemSelectionModel::SelectionFlags command)
{
    if (canChangeIndex())
        QItemSelectionModel::setCurrentIndex(index, command);
}

bool ConditionalItemSelectionModel::canChangeIndex()
{
    if (!cb)
        return true;
    quint64 time = QDateTime::currentMSecsSinceEpoch();
    if(time - lastRequestTime < 20 )
        return false;
    bool res = cb(currentIndex());
    if (!res)
        lastRequestTime = QDateTime::currentMSecsSinceEpoch();
    return res;
}


CredentialView::CredentialView(QWidget *parent) : QTreeView(parent),
    m_bIsFullyExpanded(false)
{
    setHeaderHidden(true);
    setItemDelegateForColumn(0, new ItemDelegate(this));
    setMinimumWidth(430);
    connect(this, &CredentialView::clicked, this, &CredentialView::onExpandItem);
}

CredentialView::~CredentialView()
{

}

void CredentialView::onModelLoaded()
{
    QModelIndex firstIndex = model()->index(0, 0, QModelIndex());
    if (firstIndex.isValid())
    {
        QModelIndex firstLoginIndex = firstIndex.child(0, 0);
        if (firstLoginIndex.isValid()) {
            selectionModel()->setCurrentIndex(firstLoginIndex, QItemSelectionModel::ClearAndSelect);
            expand(firstLoginIndex.parent());
        }
    }
}

void CredentialView::onExpandItem(const QModelIndex &proxyIndex)
{
    if (isExpanded(proxyIndex))
        collapse(proxyIndex);
    else
        expand(proxyIndex);
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

