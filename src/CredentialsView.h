#ifndef CREDENTIALSVIEW_H
#define CREDENTIALSVIEW_H

#include <QListView>
#include <QItemDelegate>
#include <functional>


class CredentialsView : public QListView
{
public:
    explicit CredentialsView(QWidget *parent = nullptr);
};

class ConditionalItemSelectionModel : public QItemSelectionModel {
    Q_OBJECT
public:
    using TestFunction = std::function<bool(const QModelIndex & idx)>;
    ConditionalItemSelectionModel(TestFunction f, QAbstractItemModel *model =  nullptr);

public Q_SLOTS:
    void select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) override;
    void select(const QItemSelection & selection, QItemSelectionModel::SelectionFlags command) override;
    void setCurrentIndex(const QModelIndex & index, QItemSelectionModel::SelectionFlags command) override;

private:
    bool canChangeIndex();

    TestFunction cb;
    quint64 lastRequestTime;
};

class CredentialViewItemDelegate : public QItemDelegate {

public:
    using QItemDelegate::QItemDelegate;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
};

#endif // CREDENTIALSVIEW_H
