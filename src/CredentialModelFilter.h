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
    ~CredentialModelFilter() override;
    void setFilter(const QString &sFilter);
    bool switchFavFilter();
    TreeItem *getItemByProxyIndex(const QModelIndex &proxyIndex);
    const TreeItem *getItemByProxyIndex(const QModelIndex &proxyIndex) const;
    QModelIndex getProxyIndexFromItem(TreeItem* pItem, int column = 0);
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) Q_DECL_OVERRIDE;
    QModelIndexList getNextRow(const QModelIndex &rowIdx);
    void refreshFavorites();

protected:
    virtual bool filterAcceptsRow(int iSrcRow, const QModelIndex &srcParent) const override;
    virtual bool lessThan(const QModelIndex &srcLeft, const QModelIndex &srcRight) const override;

private:
    bool acceptRow(int iSrcRow, const QModelIndex &srcParent) const;
    bool testItemAgainstNameAndDescription(TreeItem *pItem, const QString &sFilter) const;

private:
    QString m_sFilter;
    bool m_favFilter = false;
    Qt::SortOrder tempSortOrder;
};

#endif // CREDENTIALMODELFILTER_H
