// Application
#include "ServiceItem.h"
#include "LoginItem.h"

ServiceItem::ServiceItem(const QString &sServiceName):
    TreeItem(sServiceName),
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
        if ((pLoginItem != nullptr) && (pLoginItem->name().compare(sLoginName, Qt::CaseSensitive) == 0))
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
    QString sLogins = "";
    int nLogins = childCount();
    if (nLogins == 0)
        return QString();
    if (nLogins == 1)
    {
        TreeItem *pItem = m_vChilds.first();
        QString sName = pItem->name().simplified().mid(0, 15);
        if (pItem->name().length() > 15)
            sName += "...";
        sLogins = sName;
    }
    else if ((nLogins >= 1) && (nLogins <=3))
    {
        QStringList lLogins;
        foreach (TreeItem *pItem, m_vChilds)
        {
            QString sName = pItem->name().simplified().mid(0, 6);
            if (pItem->name().length() > 6)
                sName += "...";
            lLogins << sName;
        }
        sLogins = lLogins.join(", ");
    }
    else
    {
        QStringList lLogins;
        int c = 0;
        foreach (TreeItem *pItem, m_vChilds)
        {
            QString sName = pItem->name().simplified().mid(0, 3);
            if (pItem->name().length() > 3)
                sName += "...";
            lLogins << sName;
            c++;
            if (c > 10)
                break;
        }
        sLogins = lLogins.join(", ");
    }

    if (!sLogins.isEmpty())
        sLogins = QString("(")+sLogins+QString(")");
    return sLogins;
}

QDate ServiceItem::bestUpdateDate(Qt::SortOrder order) const
{
    QDate bestDate = m_dUpdatedDate;
    if (m_vChilds.length() > 0)
    {
        bestDate = m_vChilds[0]->updatedDate();
        for ( int i=1 ; i < m_vChilds.length() ; i++ )
        {
            if (order == Qt::AscendingOrder)
            {
                if (m_vChilds[i]->updatedDate() > bestDate)
                    bestDate = m_vChilds[i]->updatedDate();
            }
            else
            {
                if (m_vChilds[i]->updatedDate() < bestDate)
                    bestDate = m_vChilds[i]->updatedDate();
            }
        }
    }

    return bestDate;
}


