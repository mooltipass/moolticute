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

namespace Ui {
class CredentialsManagement;
}
class CredentialModel;
class CredentialModelFilter;
class LoginItem;

class CredentialsManagement : public QWidget
{
    Q_OBJECT

public:    
    explicit CredentialsManagement(QWidget *parent = 0);
    ~CredentialsManagement();
    void setWsClient(WSClient *c);

public slots:
    bool confirmDiscardUneditedCredentialChanges(const QModelIndex &proxyIndex = {});

private slots:
    void enableCredentialsManagement(bool);
    void updateQuickAddCredentialsButtonState();
    void onPasswordInputReturnPressed();
    void updateSaveDiscardState(const QModelIndex &proxyIndex=QModelIndex());
    void onPasswordUnlocked(const QString &service, const QString &login, const QString &password, bool success);
    void onCredentialUpdated(const QString &service, const QString &login, const QString &description, bool success);
    void saveSelectedCredential(const QModelIndex &proxyIndex=QModelIndex());
    void on_pushButtonEnterMMM_clicked();
    void on_buttonDiscard_clicked();
    void on_buttonSaveChanges_clicked();
    void requestPasswordForSelectedItem();
    void on_addCredentialButton_clicked();
    void on_pushButtonConfirm_clicked();
    void on_pushButtonCancel_clicked();
    void on_pushButtonDelete_clicked();
    void onCredentialSelected(const QModelIndex &current, const QModelIndex &previous);
    void onLoginSelected(const QModelIndex &srcIndex);
    void onServiceSelected(const QModelIndex &srcIndex);
    void onItemExpanded(const QModelIndex &proxyIndex);
    void onItemCollapsed(const QModelIndex &proxyIndex);
    void onExpandedStateChanged(bool bIsExpanded);
    void onModelLoaded(bool bClearLoginDescription);

private:
    void updateLoginDescription(const QModelIndex &srcIndex);
    void updateLoginDescription(LoginItem *pLoginItem);
    void clearLoginDescription();
    QModelIndex getSourceIndexFromProxyIndex(const QModelIndex &proxyIndex);
    QModelIndex getProxyIndexFromSourceIndex(const QModelIndex &srcIndex);

private:
    void changeCurrentFavorite(int iFavorite);
    Ui::CredentialsManagement *ui;
    CredentialModel *m_pCredModel = nullptr;
    CredentialModelFilter *m_pCredModelFilter = nullptr;
    WSClient *wsClient = nullptr;
    bool deletingCred = false;

signals:
    void wantEnterMemMode();
    void wantSaveMemMode();
    void loginSelected(const QModelIndex &srcIndex);
    void serviceSelected(const QModelIndex &srcIndex);
    void selectLoginItem(const QModelIndex &proxyIndex);
};

#endif // CREDENTIALSMANAGEMENT_H
