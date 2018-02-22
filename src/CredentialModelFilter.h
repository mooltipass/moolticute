#ifndef CREDENTIALMODELFILTER_H
#define CREDENTIALMODELFILTER_H

// Qt
#include <QSortFilterProxyModel>

// Application
class TreeItem;

class CredentialModelFilter : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    CredentialModelFilter(QObject *parent = nullptr);
    ~CredentialModelFilter();
    void setFilter(const QString &sFilter);
    TreeItem *getItemByProxyIndex(const QModelIndex &proxyIndex);
    const TreeItem *getItemByProxyIndex(const QModelIndex &proxyIndex) const;
    QModelIndex getProxyIndexFromItem(TreeItem* pItem, int column = 0);
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) Q_DECL_OVERRIDE;

protected:
    virtual bool filterAcceptsRow(int iSrcRow, const QModelIndex &srcParent) const;
    virtual bool lessThan(const QModelIndex &srcLeft, const QModelIndex &srcRight) const;

private:
    bool acceptRow(int iSrcRow, const QModelIndex &srcParent) const;
    bool testItemAgainstNameAndDescription(TreeItem *pItem, const QString &sFilter) const;

private:
    QString m_sFilter;
    Qt::SortOrder tempSortOrder;
};

#endif // CREDENTIALMODELFILTER_H
