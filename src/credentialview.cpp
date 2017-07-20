// Application
#include "credentialview.h"

//-------------------------------------------------------------------------------------------------

CredentialView::CredentialView(QWidget *parent) : QTreeView(parent)
{
    setHeaderHidden(true);
}

//-------------------------------------------------------------------------------------------------

CredentialView::~CredentialView()
{

}

//-------------------------------------------------------------------------------------------------

void CredentialView::onModelLoaded()
{
    // TO DO
}
