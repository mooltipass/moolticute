#ifndef ROOTITEM_H
#define ROOTITEM_H

// Application
#include "TreeItem.h"
class ServiceItem;

class RootItem : public TreeItem
{
public:
    RootItem();
    virtual ~RootItem();

    ServiceItem *addService(const QString &sServiceName);
    ServiceItem *findServiceByName(const QString &sServiceName);
    void setItemsStatus(const Status &eStatus);
    void removeUnusedItems();

    virtual TreeType treeType()  const Q_DECL_OVERRIDE;
};

#endif // ROOTITEM_H
