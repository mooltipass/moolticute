#ifndef TREEITEM_H
#define TREEITEM_H

// Qt
#include <QList>
#include <QVariant>
#include <QVector>
#include <QDate>

class TreeItem
{
public:
    enum Status {USED=0, UNUSED};

    //! Destructor
    virtual ~TreeItem();

    //-------------------------------------------------------------------------------------------------
    // Getters & setters
    //-------------------------------------------------------------------------------------------------

    //! Return uid
    const QString &uid() const;

    //! Return name
    const QString &name() const;

    //! Set name
    void setName(const QString &sName);

    //! Return parent item
    TreeItem *parentItem() const;

    //! Set parent item
    void setParentItem(TreeItem *pParentItem);

    //! Return status
    const Status &status() const;

    //! Set status
    void setStatus(const Status &eStatus);

    //! Return created date
    const QDate &createdDate() const;

    //! Set created date
    void setCreatedDate(const QDate &dDate);

    //! Return updated date
    const QDate &updatedDate() const;

    //! Set updated date
    void setUpdatedDate(const QDate &dDate);

    //! Return description
    const QString &description() const;

    //! Set description
    void setDescription(const QString &sDescription);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //! Retrieve child at index
    TreeItem *child(int iIndex);

    //! Return childs
    const QVector<TreeItem *> &childs() const;

    //! Return child count
    int childCount() const;

    //! Return column count
    int columnCount() const;

    //! Return parent item
    TreeItem *parentItem();

    //! Return own position
    int row() const;

    //! Return data for column
    virtual QVariant data(int iColumn) const;

    //! Add child
    void addChild(TreeItem *pItem);

    //! Remove one
    bool removeOne(TreeItem *pItem);

protected:
    //! Constructor
    explicit TreeItem(const QString &sName="",
        const QDate &dCreatedDate=QDate::currentDate(), const QDate &dUpdatedDate=QDate::currentDate(),
            const QString &setDescription="");

protected:
    //! Child vector
    QVector<TreeItem *> m_vChilds;

private:    
    //! Parent item
    TreeItem *m_pParentItem;

    //! UID
    QString m_sUID;

    //! Status
    Status m_eStatus;

    //! Name
    QString m_sName;

    //! Created date
    QDate m_dCreatedDate;

    //! Updated date
    QDate m_dUpdatedDate;

    //! Description
    QString m_sDescription;
};

#endif // TREEITEM_H
