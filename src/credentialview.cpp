// Qt
#include <QDateTime>
#include <QMouseEvent>
#include <QToolTip>
#include <QDebug>

// Application
#include "credentialview.h"
#include "CredentialModelFilter.h"
#include "serviceitem.h"

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


CredentialView::CredentialView(QWidget *parent) : QTreeView(parent)
{
    setHeaderHidden(true);
    setMouseTracking(true);
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

void CredentialView::mouseMoveEvent(QMouseEvent *event)
{
    QPoint targetPoint(128, event->y());
    QModelIndex selectedProxyIndex = indexAt(targetPoint);
    if (selectedProxyIndex != m_previousSelectedProxyIndex) {
        m_previousSelectedProxyIndex = selectedProxyIndex;
        CredentialModelFilter *pModel = dynamic_cast<CredentialModelFilter *>(model());
        TreeItem *pItem = pModel->getItemByProxyIndex(selectedProxyIndex);
        ServiceItem *pServiceItem = dynamic_cast<ServiceItem *>(pItem);
        if ((pServiceItem != nullptr) && (!pServiceItem->isExpanded()))
        {
            QString sTooltip = pServiceItem->getToolTip();
            if (!sTooltip.isEmpty())
                QToolTip::showText(mapToGlobal(targetPoint), sTooltip);
        }
        else QToolTip::hideText();
    }
}
