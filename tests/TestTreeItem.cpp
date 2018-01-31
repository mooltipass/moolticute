#include "TestTreeItem.h"
#include "../src/TreeItem.h"



TestTreeItem::TestTreeItem(QObject *parent) : QObject(parent)
{

}

void TestTreeItem::createTreeItem()
{
    BaseTreeItem *i = createBaseTreeItem();

    Q_ASSERT(i->name() == baseItemName);
    Q_ASSERT(i->accessedDate() == accessDate);
    Q_ASSERT(i->updatedDate() == updateDate);
    Q_ASSERT(i->description() == baseItemDescription);

    delete i;
}

BaseTreeItem * TestTreeItem::createBaseTreeItem()
{
    BaseTreeItem *i = new BaseTreeItem(baseItemName, updateDate, accessDate, baseItemDescription);

    return i;
}

QList<BaseTreeItem *> TestTreeItem::addChildsTo(BaseTreeItem *b)
{
    QList<BaseTreeItem*> childs;

    QString nameTemplate = "child_%1";
    BaseTreeItem *c = new BaseTreeItem();

    childs.append(c);
    b->addChild(c);

    for (int i = 0; i < 3; i++) {
        c = new BaseTreeItem(nameTemplate.arg(i));
        childs.append(c);
        b->addChild(c);
    }

    return childs;
}

void TestTreeItem::assertChilds(BaseTreeItem *b, QList<BaseTreeItem*> childs)
{
    for (TreeItem *c: b->childs()) {
        BaseTreeItem *cc = dynamic_cast<BaseTreeItem*>(c);
        Q_ASSERT(childs.contains(cc));
    }
}

void TestTreeItem::addChild()
{
    BaseTreeItem *b = createBaseTreeItem();
    QList<BaseTreeItem*> childs = addChildsTo(b);

    Q_ASSERT(b->childCount() == childs.size());

    assertChilds(b, childs);

    delete b;
}

void TestTreeItem::removeChild()
{
    BaseTreeItem *b = createBaseTreeItem();
    QList<BaseTreeItem*> childs = addChildsTo(b);

    BaseTreeItem* k = childs.first();
    childs.removeOne(k);
    b->removeOne(k);

    Q_ASSERT(b->childCount() == childs.size());

    assertChilds(b, childs);

    delete k;
    delete b;
}
