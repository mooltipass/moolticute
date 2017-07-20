#ifndef CREDENTIALVIEW_H
#define CREDENTIALVIEW_H

// Qt
#include <QTreeView>

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
