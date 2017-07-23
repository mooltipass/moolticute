// Qt
#include <QStringList>
#include <QUuid>
#include <QDebug>

// Application
#include "treeitem.h"

TreeItem::TreeItem(const QString &sName,
    const QDate &dCreatedDate, const QDate &dUpdatedDate, const QString &sDescription) :
    m_pParentItem(nullptr), m_eStatus(UNUSED),
    m_sName(sName), m_dCreatedDate(dCreatedDate), m_dUpdatedDate(dUpdatedDate),
    m_sDescription(sDescription)
{
}

TreeItem::~TreeItem()
{
    qDebug() << "DESTROYING " << this;
    qDeleteAll(m_vChilds);
}

const QString &TreeItem::name() const
{
    return m_sName;
}

void TreeItem::setName(const QString &sName)
{
    m_sName = sName;
}

TreeItem *TreeItem::child(int iIndex)
{
    if ((iIndex >= 0) && (iIndex < childCount()))
        return m_vChilds[iIndex];
    return nullptr;
}

int TreeItem::childCount() const
{
    return m_vChilds.count();
}

int TreeItem::row() const
{
    if (m_pParentItem)
        return childs().indexOf(const_cast<TreeItem*>(this));

    return 0;
}

TreeItem *TreeItem::parentItem() const
{
    return m_pParentItem;
}

void TreeItem::setParentItem(TreeItem *pParentItem)
{
    m_pParentItem = pParentItem;
}

const TreeItem::Status &TreeItem::status() const
{
    return m_eStatus;
}

void TreeItem::setStatus(const Status &eStatus)
{
    m_eStatus = eStatus;
}

const QDate &TreeItem::createdDate() const
{
    return m_dCreatedDate;
}

void TreeItem::setCreatedDate(const QDate &dDate)
{
    m_dCreatedDate = dDate;
}

const QDate &TreeItem::updatedDate() const
{
    return m_dUpdatedDate;
}

void TreeItem::setUpdatedDate(const QDate &dDate)
{
    m_dUpdatedDate = dDate;
}

const QString &TreeItem::description() const
{
    return m_sDescription;
}

void TreeItem::setDescription(const QString &sDescription)
{
    m_sDescription = sDescription;
}

const QVector<TreeItem *> &TreeItem::childs() const
{
    return m_vChilds;
}

int TreeItem::columnCount() const
{
    return 1;
}

QVariant TreeItem::data(int iColumn) const
{
    Q_UNUSED(iColumn);
    return QVariant();
}

void TreeItem::addChild(TreeItem *pItem)
{
    if (pItem != nullptr)
    {
        pItem->setParentItem(this);
        m_vChilds << pItem;
    }
}

bool TreeItem::removeOne(TreeItem *pItem)
{
    return m_vChilds.removeOne(pItem);
}

TreeItem *TreeItem::parentItem()
{
    return m_pParentItem;
}

void TreeItem::clear()
{
    qDeleteAll(m_vChilds);
    m_vChilds.clear();
}

