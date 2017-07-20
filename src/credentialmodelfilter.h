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
    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //! Constructor
    CredentialModelFilter(QObject *parent = nullptr);

    //! Destructor
    virtual ~CredentialModelFilter();

    //! Set filter
    void setFilter(const QString &sFilter);

    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //! Index to source
    int indexToSource(int idx);

    //! Index from source
    int indexFromSource(int idx);

protected:
    //-------------------------------------------------------------------------------------------------
    // Filter & sort methods
    //-------------------------------------------------------------------------------------------------

    //! Accept row
    virtual bool filterAcceptsRow(int iSrcRow, const QModelIndex &srcParent) const;

    //! Sort
    virtual bool lessThan(const QModelIndex &srcLeft, const QModelIndex &srcRight) const;

private:
    //! Accept row
    bool acceptRow(int iSrcRow, const QModelIndex &srcParent) const;

    //! Test item against name and description
    bool testItemAgainstNameAndDescription(TreeItem *pItem, const QString &sFilter) const;

private:
    //! Filter
    QString m_sFilter;
};

#endif // CREDENTIALMODELFILTER_H
