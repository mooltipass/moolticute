#ifndef SERVICEITEM_H
#define SERVICEITEM_H

// Application
#include "TreeItem.h"
class LoginItem;

class ServiceItem : public TreeItem
{
public:
    ServiceItem(const QString &sServiceName);
    virtual ~ServiceItem();

    LoginItem *addLogin(const QString &sLoginName);
    LoginItem *findLoginByName(const QString &sLoginName);
    bool isExpanded() const;
    void setExpanded(bool bExpanded);
    QString logins() const;
    QString multipleDomains() const { return m_sMultipleDomains; }
    void setMultipleDomains(const QString& domains) { m_sMultipleDomains = domains; }
    bool isMultipleDomainSet() { return !m_sMultipleDomains.isEmpty(); }

    virtual QDate bestUpdateDate(Qt::SortOrder order) const Q_DECL_OVERRIDE;
    virtual TreeType treeType()  const Q_DECL_OVERRIDE;
private:
    bool m_bIsExpanded;
    QString m_sMultipleDomains;
};

#endif // SERVICEITEM_H
