// Application
#include "RootItem.h"
#include "ServiceItem.h"

RootItem::RootItem()
{
    m_sName = "ROOT";
}

RootItem::~RootItem()
{

}

ServiceItem *RootItem::addService(const QString &sServiceName)
{
    ServiceItem *pServiceItem = new ServiceItem(sServiceName);
    addChild(pServiceItem);
    return pServiceItem;
}

ServiceItem *RootItem::findServiceByName(const QString &sServiceName)
{
    foreach (TreeItem *pItem, m_vChilds)
    {
        ServiceItem *pServiceItem = dynamic_cast<ServiceItem *>(pItem);
        if ((pServiceItem != nullptr) && (pServiceItem->name().compare(sServiceName, Qt::CaseInsensitive) == 0))
            return pServiceItem;
    }
    return nullptr;
}

void RootItem::setItemsStatus(const Status &eStatus)
{
    foreach (TreeItem *pItem, m_vChilds)
    {
        pItem->setStatus(eStatus);
        QVector<TreeItem *> vChilds = pItem->childs();
        foreach (TreeItem *pChild, vChilds)
            pChild->setStatus(eStatus);
    }
}

void RootItem::removeUnusedItems()
{
    foreach (TreeItem *pItem, m_vChilds) {
        QVector<TreeItem *> vChilds = pItem->childs();
        foreach (TreeItem *pChild, vChilds)
        {
            if (pChild->status() == UNUSED)
            {
                if (pItem->removeOne(pChild))
                    delete pChild;
            }
        }
        if (pItem->status() == UNUSED)
        {
            if (removeOne(pItem))
                delete pItem;
        }
    }
}
