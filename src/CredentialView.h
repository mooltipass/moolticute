#ifndef CREDENTIALVIEW_H
#define CREDENTIALVIEW_H

// Qt
#include <QTreeView>
#include <QTimer>

// Application
class ServiceItem;
class LoginItem;
class ItemDelegate;
class QAbstractItemModel;
class TreeItem;
class CredentialView : public QTreeView
{
    Q_OBJECT

public:
    explicit CredentialView(QWidget *parent = nullptr);
    ~CredentialView();
    void refreshLoginItem(const QModelIndex &srcIndex, bool bIsFavorite=false);

public slots:
    void onModelLoaded(bool bClearLoginDescription);

    void onToggleExpandedState(const QModelIndex &proxyIndex);
    void onChangeExpandedState();
    void onSelectionTimerTimeOut();

    virtual void setModel(QAbstractItemModel *model) Q_DECL_OVERRIDE;

private slots:
    void onLayoutChanged(const QList<QPersistentModelIndex> &parents
                            , QAbstractItemModel::LayoutChangeHint hint);

private:
    void dumpCurrentItem(const QString &intro);

    bool m_bIsFullyExpanded;
    QTimer m_tSelectionTimer;
    ServiceItem *m_pCurrentServiceItem;
    LoginItem *m_pCurrentLoginItem;
    ItemDelegate *m_pItemDelegate;

    TreeItem* tempTreeItem;
    QPersistentModelIndex tempPIndex;

signals:
    void expandedStateChanged(bool bIsExpanded);
};

#endif // CREDENTIALVIEW_H
