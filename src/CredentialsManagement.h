/******************************************************************************
 **  Copyright (c) Raoul Hecky. All Rights Reserved.
 **
 **  Moolticute is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 3 of the License, or
 **  (at your option) any later version.
 **
 **  Moolticute is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License
 **  along with Foobar; if not, write to the Free Software
 **  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 **
 ******************************************************************************/
#ifndef CREDENTIALSMANAGEMENT_H
#define CREDENTIALSMANAGEMENT_H

// Qt
#include <QtWidgets>

// Application
#include "WSClient.h"
#include "CredentialsModel.h"

namespace Ui {
class CredentialsManagement;
}
class CredentialModel;
class CredentialModelFilter;

class CredentialsManagement : public QWidget
{
    Q_OBJECT

public:    
    //-------------------------------------------------------------------------------------------------
    // Constructor & destructor
    //-------------------------------------------------------------------------------------------------

    //! Constructor
    explicit CredentialsManagement(QWidget *parent = 0);

    //! Destructor
    ~CredentialsManagement();

    //! Set WS client
    void setWsClient(WSClient *c);

public slots:
    //-------------------------------------------------------------------------------------------------
    // Public slots
    //-------------------------------------------------------------------------------------------------

    //! Confirm discard unedited credential changes
    bool confirmDiscardUneditedCredentialChanges(const QModelIndex &proxyIndex = {});

private slots:
    //-------------------------------------------------------------------------------------------------
    // Private slots
    //-------------------------------------------------------------------------------------------------

    //! Enable credentials management
    void enableCredentialsManagement(bool);

    //! Update quick add credentials button state
    void updateQuickAddCredentialsButtonState();

    //! Update save discard state
    void updateSaveDiscardState(const QModelIndex &proxyIndex=QModelIndex());

    //! Password unlocked
    void onPasswordUnlocked(const QString &service, const QString &login, const QString &password, bool success);

    //! Credential updated
    void onCredentialUpdated(const QString &service, const QString &login, const QString &description, bool success);

    //! Save selected credential
    void saveSelectedCredential(const QModelIndex &proxyIndex=QModelIndex());

    //! Button EnterMMM clicked
    void on_pushButtonEnterMMM_clicked();

    //! Button discard clicked
    void on_buttonDiscard_clicked();

    //! Button save changes clicked
    void on_buttonSaveChanges_clicked();

    //! Request password for selected item
    void requestPasswordForSelectedItem();

    //! Add credential button clicked
    void on_addCredentialButton_clicked();

    //! Confirm button clicked
    void on_pushButtonConfirm_clicked();

    //! Cancel button clicked
    void on_pushButtonCancel_clicked();

    //! Delete button clicked
    void on_pushButtonDelete_clicked();

    //! Credential selected
    void onCredentialSelected(const QModelIndex &current, const QModelIndex &previous);

    //! Login selected
    void onLoginSelected(const QString &sItemUID);

    //! Service selected
    void onServiceSelected(const QString &sItemUID);

private:
    //-------------------------------------------------------------------------------------------------
    // Control methods
    //-------------------------------------------------------------------------------------------------

    //! Update login description
    void updateLoginDescription(const QString &sItemUID);

    //! Clear login description
    void clearLoginDescription();

private:
    //! Changed current favorite
    void changeCurrentFavorite(int iFavorite);

    //! UI
    Ui::CredentialsManagement *ui;

    //! Credential model
    CredentialModel *m_pCredModel = nullptr;

    //! Credential model filter
    CredentialModelFilter *m_pCredModelFilter = nullptr;

    //! WS Client
    WSClient *wsClient = nullptr;

    //! Used when deleting a cred and not prompt user
    bool deletingCred = false;

signals:
    //-------------------------------------------------------------------------------------------------
    // Signals
    //-------------------------------------------------------------------------------------------------

    //! Want enter mem mode
    void wantEnterMemMode();

    //! Want save mem mode
    void wantSaveMemMode();

    //! Login selected
    void loginSelected(const QString &sItemUID);

    //! Service selected
    void serviceSelected(const QString &sItemUID);
};

#endif // CREDENTIALSMANAGEMENT_H
