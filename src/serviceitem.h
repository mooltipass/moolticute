#ifndef SERVICEITEM_H
#define SERVICEITEM_H

// Application
#include "treeitem.h"
class LoginItem;

class ServiceItem : public TreeItem
{
public:
    ServiceItem(const QString &sServiceName);
    virtual ~ServiceItem();
    LoginItem *addLogin(const QString &sLoginName);
    LoginItem *findLoginByName(const QString &sLoginName);
};

#endif // SERVICEITEM_H
