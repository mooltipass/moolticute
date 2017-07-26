#ifndef CREDENTIALVIEW_H
#define CREDENTIALVIEW_H

// Qt
#include <QTreeView>
#include <functional>

class ConditionalItemSelectionModel : public QItemSelectionModel {

    Q_OBJECT
public:
    using TestFunction = std::function<bool(const QModelIndex & idx)>;
    ConditionalItemSelectionModel(TestFunction f, QAbstractItemModel *model =  nullptr);
    virtual ~ConditionalItemSelectionModel();

public Q_SLOTS:
    void select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) override;
    void select(const QItemSelection & selection, QItemSelectionModel::SelectionFlags command) override;
    void setCurrentIndex(const QModelIndex & index, QItemSelectionModel::SelectionFlags command) override;

private:
    bool canChangeIndex();

private:
    TestFunction cb;
    quint64 lastRequestTime;
};

class CredentialView : public QTreeView
{
    Q_OBJECT

public:
    explicit CredentialView(QWidget *parent = nullptr);
    virtual ~CredentialView();

public slots:
    void onModelLoaded();
    void onExpandItem(const QModelIndex &proxyIndex);
    void onChangeExpandedState();

private:
    bool m_bIsFullyExpanded;

signals:
    void expandedStateChanged(bool bIsExpanded);
};

#endif // CREDENTIALVIEW_H
