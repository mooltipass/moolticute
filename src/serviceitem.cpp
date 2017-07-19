// Application
#include "serviceitem.h"
#include "loginitem.h"

//-------------------------------------------------------------------------------------------------

ServiceItem::ServiceItem(const QString &sServiceName) : TreeItem(SERVICE, sServiceName)
{

}

//-------------------------------------------------------------------------------------------------

ServiceItem::~ServiceItem()
{

}

//-------------------------------------------------------------------------------------------------

LoginItem *ServiceItem::addLogin(const QString &sLoginName)
{
    LoginItem *pLoginItem = new LoginItem(sLoginName);
    addChild(pLoginItem);
    return pLoginItem;
}

//-------------------------------------------------------------------------------------------------

LoginItem *ServiceItem::findLoginByName(const QString &sLoginName)
{
    foreach (TreeItem *pItem, m_vChilds) {
        LoginItem *pLoginItem = dynamic_cast<LoginItem *>(pItem);
        if ((pLoginItem != nullptr) && (pLoginItem->name().compare(sLoginName, Qt::CaseInsensitive) == 0))
            return pLoginItem;
    }
    return nullptr;
}
