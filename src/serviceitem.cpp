// Application
#include "ServiceItem.h"
#include "LoginItem.h"

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

QString ServiceItem::logins() const
{
    int nLogins = childCount();
    if (nLogins == 0)
        return QString();
    if (nLogins == 1)
    {
        TreeItem *pItem = m_vChilds.first();
        QString sName = pItem->name().simplified().mid(0, 15);
        if (pItem->name().length() > 15)
            sName += "...";
        return sName;
    }
    else
        if ((nLogins >= 1) && (nLogins <=3))
        {
            QStringList lLogins;
            foreach (TreeItem *pItem, m_vChilds)
            {
                QString sName = pItem->name().simplified().mid(0, 6);
                if (pItem->name().length() > 6)
                    sName += "...";
                lLogins << sName;
            }
            return lLogins.join(" ");
        }
        else
        {
            QStringList lLogins;
            foreach (TreeItem *pItem, m_vChilds)
            {
                QString sName = pItem->name().simplified().mid(0, 3);
                if (pItem->name().length() > 3)
                    sName += "...";
                lLogins << sName;
            }
            return lLogins.join(" ");
        }

    return QString();
}


