#ifndef SERVICEITEM_H
#define SERVICEITEM_H

// Application
#include "treeitem.h"
class LoginItem;

class ServiceItem : public TreeItem
{
public:
    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //! Constructor
    ServiceItem(const QString &sServiceName);

    //! Destructor
    virtual ~ServiceItem();

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //! Add login
    LoginItem *addLogin(const QString &sLoginName);

    //! Find login by name
    LoginItem *findLoginByName(const QString &sLoginName);
};

#endif // SERVICEITEM_H
