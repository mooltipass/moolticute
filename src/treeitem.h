#ifndef TREEITEM_H
#define TREEITEM_H

#include <QList>
#include <QVariant>
#include <QVector>

class TreeItem
{
public:
    enum Type {ROOT=0, SERVICE, LOGIN};
    enum Status {USED=0, UNUSED};

    //! Destructor
    virtual ~TreeItem();

    //-------------------------------------------------------------------------------------------------
    // Getters & setters
    //-------------------------------------------------------------------------------------------------

    //! Return uid
    const QString &uid() const;

    //! Return type
    const Type &type() const;

    //-------------------------------------------------------------------------------------------------
    // Getters
    //-------------------------------------------------------------------------------------------------

    //! Retrieve child at index
    TreeItem *child(int iIndex);

    //! Return child count
    int childCount() const;

    //! Return column count
    int columnCount() const;

    //! Return parent item
    TreeItem *parentItem();

    //! Return own position
    int row() const;

    //! Return name
    const QString &name() const;

    //-------------------------------------------------------------------------------------------------
    // Setters
    //-------------------------------------------------------------------------------------------------

    //! Set parent item
    void setParentItem(TreeItem *pParentItem);

    //! Set status
    void setStatus(const Status &eStatus);

    //! Set name
    void setName(const QString &sName);

    //-------------------------------------------------------------------------------------------------
    // Getters
    //-------------------------------------------------------------------------------------------------

    //! Return status
    const Status &status() const;

    //! Return childs
    const QVector<TreeItem *> &childs() const;

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //! Return data for column
    virtual QVariant data(int iColumn) const;

    //! Add child
    void addChild(TreeItem *pItem);

    //! Remove one
    bool removeOne(TreeItem *pItem);

protected:
    //! Constructor
    explicit TreeItem(const Type &eType=ROOT, const QString &sName="");

protected:
    //! Child vector
    QVector<TreeItem *> m_vChilds;

private:
    //! UID
    QString m_sUID;

    //! Type
    Type m_eType;

    //! Parent item
    TreeItem *m_pParentItem;

    //! Status
    Status m_eStatus;

    //! Name
    QString m_sName;
};

#endif // TREEITEM_H
