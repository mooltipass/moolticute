#ifndef CREDENTIALVIEW_H
#define CREDENTIALVIEW_H

// Qt
#include <QTreeView>
#include <functional>

class ConditionalItemSelectionModel : public QItemSelectionModel {

    Q_OBJECT
public:
    using TestFunction = std::function<bool(const QModelIndex & idx)>;

    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //! Constructor
    ConditionalItemSelectionModel(TestFunction f, QAbstractItemModel *model =  nullptr);

    //! Destructor
    virtual ~ConditionalItemSelectionModel();

public Q_SLOTS:
    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //! Select
    void select(const QModelIndex &index, QItemSelectionModel::SelectionFlags command) override;

    //! Select
    void select(const QItemSelection & selection, QItemSelectionModel::SelectionFlags command) override;

    //! Set current index
    void setCurrentIndex(const QModelIndex & index, QItemSelectionModel::SelectionFlags command) override;

private:
    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //! Can change index?
    bool canChangeIndex();

private:
    //! Test function
    TestFunction cb;

    //! Last request time
    quint64 lastRequestTime;
};

class CredentialView : public QTreeView
{
    Q_OBJECT

public:
    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //! Constructor
    explicit CredentialView(QWidget *parent = nullptr);

    //! Destructor
    virtual ~CredentialView();

public slots:
    //-------------------------------------------------------------------------------------------------
    // Public slots
    //-------------------------------------------------------------------------------------------------

    //! Model loaded
    void onModelLoaded();
};

#endif // CREDENTIALVIEW_H
