#ifndef TESTTREEITEM_H
#define TESTTREEITEM_H

#include <QObject>
#include "../src/TreeItem.h"

class BaseTreeItem : public TreeItem {
public:
    explicit BaseTreeItem(const QString &sName = "",
                          const QDate &dCreatedDate = QDate::currentDate(),
                          const QDate &dUpdatedDate = QDate::currentDate(),
                          const QString &setDescription = "") :
        TreeItem(sName, dCreatedDate, dUpdatedDate, setDescription) {
    }

    ~BaseTreeItem() {
    }
};

class TestTreeItem : public QObject
{
    Q_OBJECT
public:
    explicit TestTreeItem(QObject *parent = nullptr);

    BaseTreeItem * createBaseTreeItem();

    void assertChilds(BaseTreeItem *b, QList<BaseTreeItem*> childs);

private Q_SLOTS:
    void createTreeItem();
    void addChild();
    void removeChild();
private:
    QString baseItemName = "base";
    QString baseItemDescription = "base item";
    QDate accessDate = QDate(2018,1, 28);
    QDate updateDate = QDate(2018,1, 1);
    QList<BaseTreeItem *> addChildsTo(BaseTreeItem *b);
};

#endif // TESTTREEITEM_H
