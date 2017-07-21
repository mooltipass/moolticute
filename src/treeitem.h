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
    virtual ~TreeItem();
    const QString &uid() const;
    const QString &name() const;
    void setName(const QString &sName);
    TreeItem *parentItem() const;
    void setParentItem(TreeItem *pParentItem);
    const Status &status() const;
    void setStatus(const Status &eStatus);
    const QDate &createdDate() const;
    void setCreatedDate(const QDate &dDate);
    const QDate &updatedDate() const;
    void setUpdatedDate(const QDate &dDate);
    const QString &description() const;
    void setDescription(const QString &sDescription);
    TreeItem *child(int iIndex);
    const QVector<TreeItem *> &childs() const;
    int childCount() const;
    int columnCount() const;
    TreeItem *parentItem();
    int row() const;
    virtual QVariant data(int iColumn) const;
    void addChild(TreeItem *pItem);
    bool removeOne(TreeItem *pItem);
    void clear();

protected:
    explicit TreeItem(const QString &sName="",
        const QDate &dCreatedDate=QDate::currentDate(), const QDate &dUpdatedDate=QDate::currentDate(),
            const QString &setDescription="");

protected:
    QVector<TreeItem *> m_vChilds;

private:    
    TreeItem *m_pParentItem;
    QString m_sUID;
    Status m_eStatus;
    QString m_sName;
    QDate m_dCreatedDate;
    QDate m_dUpdatedDate;
    QString m_sDescription;
};

#endif // TREEITEM_H
