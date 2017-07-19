#ifndef ROOTITEM_H
#define ROOTITEM_H

// Application
#include "treeitem.h"
class ServiceItem;

class RootItem : public TreeItem
{
public:
    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //! Constructor
    RootItem();

    //! Destructor
    virtual ~RootItem();

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //! Add service
    ServiceItem *addService(const QString &sServiceName);

    //! Find service by name
    ServiceItem *findServiceByName(const QString &sServiceName);

    //! Set items status
    void setItemsStatus(const Status &eStatus);

    //! Remove unused items
    void removeUnusedItems();
};

#endif // ROOTITEM_H
