#ifndef CREDENTIALVIEW_H
#define CREDENTIALVIEW_H

// Qt
#include <QTreeView>
#include <QTimer>

// Application
class ServiceItem;

class CredentialView : public QTreeView
{
    Q_OBJECT

public:
    explicit CredentialView(QWidget *parent = nullptr);
    ~CredentialView();
    void refreshLoginItem(const QModelIndex &srcIndex);

public slots:
    void onModelLoaded(bool bClearLoginDescription);
    void onToggleExpandedState(const QModelIndex &proxyIndex);
    void onChangeExpandedState();
    void onSelectionTimerTimeOut();

private:
    bool m_bIsFullyExpanded;
    QTimer m_tSelectionTimer;
    ServiceItem *m_pCurrentServiceItem;

signals:
    void expandedStateChanged(bool bIsExpanded);
};

#endif // CREDENTIALVIEW_H
