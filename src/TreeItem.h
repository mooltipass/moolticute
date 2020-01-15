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
    enum TreeType {Root = 0, Service, Login, Base};
    virtual ~TreeItem();

    const QString &name() const;
    void setName(const QString &sName);
    TreeItem *parentItem() const;
    void setParentItem(TreeItem *pParentItem);
    const Status &status() const;
    void setStatus(const Status &eStatus);
    const QDate &updatedDate() const;
    virtual QDate bestUpdateDate(Qt::SortOrder order) const;
    void setUpdatedDate(const QDate &dDate);
    const QDate &accessedDate() const;
    void setAccessedDate(const QDate &dDate);
    const QString &description() const;
    void setDescription(const QString &sDescription);
    int category() const;
    void setCategory(int iCategory);
    int keyAfterLogin() const;
    void setkeyAfterLogin(int key);
    int keyAfterPwd() const;
    void setkeyAfterPwd(int key);
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
    virtual TreeType treeType()  const;

protected:
    explicit TreeItem(const QString &sName = "",
                      const QDate &dCreatedDate = QDate::currentDate(),
                      const QDate &dUpdatedDate = QDate::currentDate(),
                      const QString &setDescription = "");

protected:
    QVector<TreeItem *> m_vChilds;
    TreeItem *m_pParentItem;
    Status m_eStatus;
    QString m_sName;
    QDate m_dUpdatedDate;
    QDate m_dAccessedDate;
    QString m_sDescription;
    int m_iCategory;
    int m_iKeyAfterLogin = 0xFFFF;
    int m_iKeyAfterPwd = 0xFFFF;
};

#endif // TREEITEM_H
