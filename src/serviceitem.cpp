// Application
#include "serviceitem.h"
#include "loginitem.h"

ServiceItem::ServiceItem(const QString &sServiceName) : TreeItem(sServiceName),
    m_bIsExpanded(false)
{

}

ServiceItem::~ServiceItem()
{

}

LoginItem *ServiceItem::addLogin(const QString &sLoginName)
{
    LoginItem *pLoginItem = new LoginItem(sLoginName);
    addChild(pLoginItem);
    return pLoginItem;
}

LoginItem *ServiceItem::findLoginByName(const QString &sLoginName)
{
    foreach (TreeItem *pItem, m_vChilds)
    {
        LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);
        if ((pLoginItem != nullptr) && (pLoginItem->name().compare(sLoginName, Qt::CaseInsensitive) == 0))
            return pLoginItem;
    }
    return nullptr;
}

bool ServiceItem::isExpanded() const
{
    return m_bIsExpanded;
}

void ServiceItem::setExpanded(bool bExpanded)
{
    m_bIsExpanded = bExpanded;
}

QString ServiceItem::getToolTip() const
{
    QStringList lLoginNames;
    foreach (TreeItem *pLoginItem, m_vChilds)
        lLoginNames << pLoginItem->name();
    if (!lLoginNames.isEmpty())
        return lLoginNames.join("\n");
    return QString("");
}
