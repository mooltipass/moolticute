// Qt
#include <QStringList>
#include <QDebug>

// Application
#include "TreeItem.h"

TreeItem::TreeItem(const QString &sName, const QDate &dCreatedDate, const QDate &dUpdatedDate, const QString &sDescription) :
    m_pParentItem(nullptr),
    m_eStatus(UNUSED),
    m_sName(sName),
    m_dUpdatedDate(dCreatedDate),
    m_dAccessedDate(dUpdatedDate),
    m_sDescription(sDescription)
{
}

TreeItem::~TreeItem()
{
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
        return m_pParentItem->childs().indexOf(const_cast<TreeItem*>(this));

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

const QDate &TreeItem::updatedDate() const
{
    return m_dUpdatedDate;
}

QDate TreeItem::bestUpdateDate(Qt::SortOrder) const
{
    return m_dUpdatedDate;
}

void TreeItem::setUpdatedDate(const QDate &dDate)
{
    m_dUpdatedDate = dDate;
}

const QDate &TreeItem::accessedDate() const
{
    return m_dAccessedDate;
}

void TreeItem::setAccessedDate(const QDate &dDate)
{
    m_dAccessedDate = dDate;
}

const QString &TreeItem::description() const
{
    return m_sDescription;
}

void TreeItem::setDescription(const QString &sDescription)
{
    m_sDescription = sDescription;
}

int TreeItem::category() const
{
    return m_iCategory;
}

void TreeItem::setCategory(int iCategory)
{
    m_iCategory = iCategory;
}

int TreeItem::keyAfterLogin() const
{
    return m_iKeyAfterLogin;
}

void TreeItem::setkeyAfterLogin(int key)
{
    m_iKeyAfterLogin = key;
}

int TreeItem::keyAfterPwd() const
{
    return m_iKeyAfterPwd;
}

void TreeItem::setkeyAfterPwd(int key)
{
    m_iKeyAfterPwd = key;
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

TreeItem::TreeType TreeItem::treeType() const
{
    return Base;
}
