// Qt
#include <QStringList>
#include <QUuid>

// Application
#include "treeitem.h"

//-------------------------------------------------------------------------------------------------

TreeItem::TreeItem(const Type &eType, const QString &sName) : m_sUID(QUuid::createUuid().toString()), m_eType(eType),
    m_pParentItem(nullptr), m_eStatus(UNUSED), m_sName(sName)
{
}

//-------------------------------------------------------------------------------------------------

TreeItem::~TreeItem()
{
    qDeleteAll(m_vChilds);
}

//-------------------------------------------------------------------------------------------------

const QString &TreeItem::uid() const
{
    return m_sUID;
}

//-------------------------------------------------------------------------------------------------

const TreeItem::Type &TreeItem::type() const
{
    return m_eType;
}

//-------------------------------------------------------------------------------------------------

TreeItem *TreeItem::child(int iIndex)
{
    if ((iIndex >= 0) && (iIndex < childCount()))
        return m_vChilds[iIndex];
    return nullptr;
}

//-------------------------------------------------------------------------------------------------

int TreeItem::childCount() const
{
    return m_vChilds.count();
}

//-------------------------------------------------------------------------------------------------

int TreeItem::row() const
{
    if (m_pParentItem)
        return m_pParentItem->m_vChilds.indexOf(const_cast<TreeItem*>(this));

    return 0;
}

//-------------------------------------------------------------------------------------------------

const QString &TreeItem::name() const
{
    return m_sName;
}

//-------------------------------------------------------------------------------------------------

void TreeItem::setName(const QString &sName)
{
    m_sName = sName;
}

//-------------------------------------------------------------------------------------------------

void TreeItem::setParentItem(TreeItem *pParentItem)
{
    m_pParentItem = pParentItem;
}

//-------------------------------------------------------------------------------------------------

void TreeItem::setStatus(const Status &eStatus)
{
    m_eStatus = eStatus;
}

//-------------------------------------------------------------------------------------------------

const TreeItem::Status &TreeItem::status() const
{
    return m_eStatus;
}

//-------------------------------------------------------------------------------------------------

const QVector<TreeItem *> &TreeItem::childs() const
{
    return m_vChilds;
}

//-------------------------------------------------------------------------------------------------

int TreeItem::columnCount() const
{
    return 1;
}

//-------------------------------------------------------------------------------------------------

QVariant TreeItem::data(int iColumn) const
{
    Q_UNUSED(iColumn);
    return QVariant();
}

//-------------------------------------------------------------------------------------------------

void TreeItem::addChild(TreeItem *pItem)
{
    if (pItem != nullptr) {
        pItem->setParentItem(this);
        m_vChilds << pItem;
    }
}

//-------------------------------------------------------------------------------------------------

bool TreeItem::removeOne(TreeItem *pItem)
{
    return m_vChilds.removeOne(pItem);
}

//-------------------------------------------------------------------------------------------------

TreeItem *TreeItem::parentItem()
{
    return m_pParentItem;
}

